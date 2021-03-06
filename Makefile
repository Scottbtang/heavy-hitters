# Shell
SHELL = /bin/bash

# Compiler
CC = gcc
#CC = clang

# Debug or Release
PROFILE = -Og -g -DRUPIFY -DNDEBUG
DEBUG = -Og -g
RELEASE = -O3 -DNDEBUG
SPACE = -Os -DNDEBUG
EXEC = ${RELEASE}

# Which binary to compile
NAME = precision_hh precision_sketch benchmark_hh benchmark_sketch error_sketch

# Compiler options
CFLAGS = -MMD -pipe -fno-exceptions -fstack-protector\
		-fvisibility=hidden -W -Wall -Wno-unused-parameter\
		-Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat\
		-Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral\
		-Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -Werror\
		-Winline -pedantic -pedantic-errors ${EXEC} -fPIC -march=native \
		-funroll-loops -falign-functions=16 -freorder-blocks ${USERFLAGS}

CVER = -std=c99

DIR = $(shell pwd)

SRC_FOLDER     = ${DIR}/src
BUILD_FOLDER   = ${DIR}/build
BIN_FOLDER     = ${DIR}/bin
TESTS_FOLDER   = ${DIR}/tests
MODULES_FOLDER = ${DIR}/modules
UTIL_FOLDER    = ${DIR}/util

# FLAGS
FLAGS_GENERAL = -I${SRC_FOLDER} -I${MODULES_FOLDER} -I${UTIL_FOLDER}
FLAGS_LD      = -Wl,-z,relro -Wl,-z,now -lm -L ${MODULES_FOLDER}/libmeasure \
				-lmeasure -Wl,-rpath=${MODULES_FOLDER}/libmeasure
LD_TEST       = -lcriterion
FLAGS_TEST    = -I ${SRC_FOLDER}

# First level of directories in src folder
SRC_DIRS := $(subst /.,,$(subst ${SRC_FOLDER},,$(wildcard ${SRC_FOLDER}/*/.)))

# all c source files
SRC   := $(shell find ${SRC_FOLDER} -name "*.c" -type f;)

# all test files
TESTS := $(shell find ${TESTS_FOLDER} -name "test_*.c" -type f;)

# non-test obect files
OBJ  = ${subst ${SRC_FOLDER}, ${BUILD_FOLDER}, ${patsubst %.c, %.o, $(SRC)}}

# non-test assembly files
ASM  = ${subst ${SRC_FOLDER}, ${BUILD_FOLDER}, ${patsubst %.c, %.S, $(SRC)}}

# test object files
TEST = ${subst ${TESTS_FOLDER}, ${BUILD_FOLDER}, ${patsubst %.c, %.o, $(TESTS)}}

# names of tests
TEST_NAMES = ${patsubst ${TESTS_FOLDER}/%.c, %, ${TESTS}}

# all object files
ALL  = ${OBJ} ${TEST}

# object files and .d files
DEPS = ${ALL:%.o=%.d}

# log name
LOG = VALGRIND_LOG

# what we are trying to build
all:  build $(NAME)
asm:  build $(OBJ) $(ASM)

test: bin build $(TEST_NAMES) ${addprefix run_,${TEST_NAMES}}

# Recompile when headers change
-include $(DEPS)

build:
	echo $(wildcard ${SRC_FOLDER}/*/.);
	if [[ ! -e ${BUILD_FOLDER} ]]; then mkdir -p ${BUILD_FOLDER}; fi
	mkdir $(addprefix $(BUILD_FOLDER), ${SRC_DIRS})

bin:
	if [[ ! -e ${BIN_FOLDER} ]]; then mkdir -p ${BIN_FOLDER}; fi

$(NAME): ${OBJ}
	@echo
	@echo ================ [Linking] ================
	@echo
	$(CC) ${CFLAGS} ${CVER} -o $@ \
		$(filter-out $(filter-out ${BUILD_FOLDER}/$@.o, $(addsuffix .o, $(addprefix ${BUILD_FOLDER}/, ${NAME}))), $^) \
		${FLAGS_LD} $(FLAGS_GENERAL)
	@echo
	@echo ================ [$@ compiled succesfully] ================
	@echo

$(BUILD_FOLDER)/%.S: $(SRC_FOLDER)/%.c
	@echo
	@echo ================ [Building Assembly] ================
	@echo
	$(CC) $(CFLAGS) $(FLAGS_GENERAL) $(CVER) -c $< -S -o $@
	@echo
	@echo OK [$<] - [$@]
	@echo

# compile every source file
$(BUILD_FOLDER)/%.o: $(SRC_FOLDER)/%.c
	@echo
	@echo ================ [Building Object] ================
	@echo
	$(CC) $(CFLAGS) $(FLAGS_GENERAL) $(CVER) -c $< -o $@
	@echo
	@echo OK [$<] - [$@]
	@echo

$(BUILD_FOLDER)/%.o: $(TESTS_FOLDER)/%.c
	@echo
	@echo ================ [Building Object] ================
	@echo
	$(CC) $(FLAGS_TEST) $(CFLAGS) $(CVER) -c $< -o $@
	@echo
	@echo OK [$<] - [$@]
	@echo

valgrind: clean all
	@echo
	@echo ================ [Executing $(NAME) using Valgrind] ================
	@echo
	valgrind -v --leak-check=full --log-file="$(LOG)" --track-origins=yes \
	--show-reachable=yes ./$(NAME)
	@echo
	@echo ================ [Log] ================
	@echo
	less $(LOG)
	@echo

run: clean all
	@echo
	@echo ================ [Executing $(NAME)] ================
	@echo
	./$(NAME)

clean:
	@echo $(SRCS)
	@echo
	@echo ================ [Cleaning $(NAME)] ================
	@echo
	rm -f ${OBJ} ${TEST} ${DEPS} ${ASM} ${addprefix bin/,${TEST_NAMES}}
	rm -f gmon.out callgrind.* $(LOG) ${TESTS:.c=} ${NAME}
	if [[ -d build ]] ; then rmdir --ignore-fail-on-non-empty $(addprefix ${BUILD_FOLDER}, ${SRC_DIRS}); rmdir --ignore-fail-on-non-empty build; fi
	if [[ -d bin ]] ; then rmdir --ignore-fail-on-non-empty bin; fi
	#$(MAKE) -C libmeasure clean

count:
	@echo
	@echo ================ [Counting lines in $(NAME)] ================
	@echo
	sloccount --wide .

callgrind:
	@echo
	@echo ================ [Profiling $(NAME)] ================
	@echo
	valgrind --tool=callgrind ./$(NAME)

${TEST_NAMES}: bin build ${OBJ} ${TEST}
	@echo
	@echo ================ [Linking Tests] ================
	@echo
	$(CC) ${CFLAGS} ${CVER} -o ${BIN_FOLDER}/$@ ${BUILD_FOLDER}/$@.o\
		$(filter-out $(addsuffix .o, $(addprefix ${BUILD_FOLDER}/, ${NAME})), $(filter-out ${BUILD_FOLDER}/test_%.o, $(ALL)))\
		${FLAGS_LD} $(FLAGS_GENERAL) $(LD_TEST)
	@echo
	@echo ================ [$@ compiled succesfully] ================

${addprefix run_,${TEST_NAMES}}: ${TEST_NAMES}
	@echo ================ [Running test ${patsubst run_%,%,$@}] ================
	@echo
	${BIN_FOLDER}/${patsubst run_%,%,$@} --verbose --no-early-exit
