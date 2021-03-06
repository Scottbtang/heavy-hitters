#!/usr/bin/env bash

set -e
set -u

OUT=$1
EXTRA=${2:-}

while [[ $# > 1 ]]
do
	key="$2"

	case $key in
		-f|--file)
			FILE="$3"
			shift # past argument
			;;
		-s|--sketch)
			TYPE="sketch"
			shift # past argument
			;;
		-m|--universe)
			UNIVERSE=$3
			shift # past argument
			;;
		*)
			# unknown option
			;;
	esac
	shift # past argument or value
done

# Run heavy hitter if sketch is not defined
: "${TYPE:="hh"}"

# Universe
: "${UNIVERSE:=2147483647}"

function ceil {
	res=$(echo "($1 + 0.5)/1" | bc)
	if [ $(echo "${res} < $1" | bc) -eq 1 ]; then
		res=$((${res}+1))
	fi
	echo ${res}
}

# HEIGHT and WIDTH is those satisfying the Count Min Sketch guarantees
if [ "$TYPE" == "hh" ]; then
	limit=$(echo "2^12" | bc)
	for ((i=2; i<=limit; i*=2));
	do
		B=4
		DELTA=0.25
		PHI=$(echo "1/${i}" | bc -l)
		EPSILON=$(echo "1/(${i}*2)" | bc -l)

		echo -n "# COMMIT: " >> ${OUT}.${i}.const
		git log -1 --oneline >> ${OUT}.${i}.const
		echo -n "# "         >> ${OUT}.${i}.const
		date                 >> ${OUT}.${i}.const

		echo -n "# COMMIT: " >> ${OUT}.${i}.min
		git log -1 --oneline >> ${OUT}.${i}.min
		echo -n "# "         >> ${OUT}.${i}.min
		date                 >> ${OUT}.${i}.min

		echo -n "# COMMIT: " >> ${OUT}.${i}.median
		git log -1 --oneline >> ${OUT}.${i}.median
		echo -n "# "         >> ${OUT}.${i}.median
		date                 >> ${OUT}.${i}.median

		echo -n "# COMMIT: " >> ${OUT}.${i}.cmh
		git log -1 --oneline >> ${OUT}.${i}.cmh
		echo -n "# "         >> ${OUT}.${i}.cmh
		date                 >> ${OUT}.${i}.cmh

		echo -n "# COMMIT: " >> ${OUT}.${i}.kmin
		git log -1 --oneline >> ${OUT}.${i}.kmin
		echo -n "# "         >> ${OUT}.${i}.kmin
		date                 >> ${OUT}.${i}.kmin

		echo -n "# COMMIT: " >> ${OUT}.${i}.kmedian
		git log -1 --oneline >> ${OUT}.${i}.kmedian
		echo -n "# "         >> ${OUT}.${i}.kmedian
		date                 >> ${OUT}.${i}.kmedian

		h=$(echo "scale=10;l((2*(l(${UNIVERSE})/l(2)))/(${PHI}*${DELTA}))/l(${B})" | bc -l)
		w=$(echo "${B}/${EPSILON}" | bc -l)

		SEED1=$[ 1 + $[ RANDOM % 32768 ]]
		SEED2=$[ 1 + $[ RANDOM % 32768 ]]
		RUNS=1
		WIDTH=$(ceil ${w})
		HEIGHT=$(ceil ${h})

#		echo "Height: ${HEIGHT}"
#		echo "Width: ${WIDTH}"


		./benchmark_hh -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} -w ${WIDTH} \
			-h ${HEIGHT} -p ${PHI} -d ${DELTA} -f ${FILE} --const  -r ${RUNS} \
			-o ${OUT}.${i}.const
		./benchmark_hh -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} -w ${WIDTH} \
			-h ${HEIGHT} -p ${PHI} -d ${DELTA} -f ${FILE} --min    -r ${RUNS} \
			-o ${OUT}.${i}.min
		./benchmark_hh -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} -w ${WIDTH} \
			-h ${HEIGHT} -p ${PHI} -d ${DELTA} -f ${FILE} --median -r ${RUNS} \
			-o ${OUT}.${i}.median
		./benchmark_hh -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} -w ${WIDTH} \
			-h ${HEIGHT} -p ${PHI} -d ${DELTA} -f ${FILE} --cormode -r ${RUNS} \
			-o ${OUT}.${i}.cmh
		./benchmark_hh -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} -w ${WIDTH} \
			-h ${HEIGHT} -p ${PHI} -d ${DELTA} -f ${FILE} --kmin    -r ${RUNS} \
			-o ${OUT}.${i}.kmin
		./benchmark_hh -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} -w ${WIDTH} \
			-h ${HEIGHT} -p ${PHI} -d ${DELTA} -f ${FILE} --kmedian -r ${RUNS} \
			-o ${OUT}.${i}.kmedian
	done
else
	limit=$(echo "2^10" | bc)
	e=2
	for ((i=1; e<limit; i++));
	do
		B=4
		SEED1=$[ 1 + $[ RANDOM % 32768 ]]
		SEED2=$[ 1 + $[ RANDOM % 32768 ]]
		EPSILON=$(echo "(1/${e})" | bc -l)
		DELTA=$(echo "(1/(2^18))" | bc -l)

		h=$(echo "scale=10;l(1/${DELTA})/l(${B})" | bc -l)
		w=$(echo "${B}/${EPSILON}" | bc -l)

		WIDTH=$(ceil ${w})
		HEIGHT=$(ceil ${h})

		RUNS=5

		echo -n "# COMMIT: " >> ${OUT}.${e}.min
		git log -1 --oneline >> ${OUT}.${e}.min
		echo -n "# "         >> ${OUT}.${e}.min
		date                 >> ${OUT}.${e}.min

		echo -n "# COMMIT: " >> ${OUT}.${e}.median
		git log -1 --oneline >> ${OUT}.${e}.median
		echo -n "# "         >> ${OUT}.${e}.median
		date                 >> ${OUT}.${e}.median

		# Measure Theoretically
		./benchmark_sketch -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} \
			-d ${DELTA} -e ${EPSILON} -f ${FILE} --min \
			-r ${RUNS} -o ${OUT}.${e}.min
		./benchmark_sketch -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} \
			-d ${DELTA} -e ${EPSILON} -f ${FILE} --median \
			-r ${RUNS} -o ${OUT}.${e}.median

		# Measure Equally
	#	./benchmark_sketch -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} \
	#		-w ${WIDTH} -h ${HEIGHT} -f ${FILE} --min \
	#		-r ${RUNS} -o ${OUT}.${e}.min
	#	./benchmark_sketch -m ${UNIVERSE} -1 ${SEED1} -2 ${SEED2} \
	#		-w ${WIDTH} -h ${HEIGHT} -f ${FILE} --median \
	#		-r ${RUNS} -o ${OUT}.${e}.median

		((e*=2))
	done
fi
