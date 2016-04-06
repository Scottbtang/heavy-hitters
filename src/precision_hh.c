#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "stream/stream.h"
#include "sketch/sketch.h"
#include "hh/hh.h"
#include "hh/sketch.h"
#include "util/xutil.h"

#define AMOUNT_OF_IMPLEMENTATIONS 3

typedef struct {
	double   timestamp;
	uint16_t size;
	char     sourceIP[16];
	char     destinationIP[16];
	uint16_t sourcePort;
	uint16_t destinationPort;
	char     flags[5];
	uint8_t  protocol;
	uint8_t  direction;
	char     type[3];
	uint32_t random;
} NUST_t;

typedef struct {
	uint32_t id;
	char     date[11];
	char     time[9];
	char     duration[9];
	char     serv[256];
	char     sourcePort[6];
	char     destinationPort[6];
	char     sourceIP[16];
	char     destinationIP[16];
	double   attack_score;
	char     attack_name[256];
} DARPA_t;

typedef enum {
	NUST,
	DARPA,
	BINARY,
} format_t;

typedef enum {
	MIN,
	MEDIAN,
	CONST,
} hh_impl_t;

typedef struct {
	hh_impl_t impl;
	int       index;
} alg_t;

static void printusage(char *argv[]) {
    fprintf(stderr, "Usage: %s \n"
            "\t[-f --file     [char *]   {REQUIRED} (Filename to hand to stream)]\n"
            "\t[-e --epsilon  [double]   {OPTIONAL} (Epsilon value)]\n"
            "\t[-d --delta    [double]   {OPTIONAL} (Delta value)]\n"
            "\t[-p --phi      [double]   {OPTIONAL} (Phi value)]\n"
            "\t[-m --universe [uint32_t] {OPTIONAL} (Universe i.e. amount of unique items)]\n"
            "\t[-w --width    [double]   {OPTIONAL} (Height of sketch)]\n"
            "\t[-h --height   [double]   {OPTIONAL} (Width of sketch)]\n"
            "\t[--min                    {OPTIONAL} (Run HH with count-min-sketch)]\n"
            "\t[--median                 {OPTIONAL} (Run HH with count-median-sketch)]\n"
            "\t[--const                  {OPTIONAL} (Run HH with constant-count-min-sketch)]\n"
            "\t[-1 --seed1    [uint32_t] {OPTIONAL} (First seed value)]\n"
            "\t[-2 --seed2    [uint32_t] {OPTIONAL} (Second seed value)]\n"
            "\t[-i --info                {OPTIONAL} (Shows this guideline)]\n"
            , argv[0]);
}

int main (int argc, char **argv) {
	char     *string;
	char      buf[256];
	char     *buffer;
	uint32_t  i, j, k, uid;
	uint8_t   h1, h2, h3, h4;
	stream_t *stream;
	int32_t   opt, c   = 0, d = 0;
	format_t  format   = -1;
	char     *filename = NULL;
	uint64_t  buf_size = 0;
	bool start         = true;

	alg_t     alg[AMOUNT_OF_IMPLEMENTATIONS];
	hh_t     *impl[AMOUNT_OF_IMPLEMENTATIONS];
	uint8_t   impl_cnt = 0;
	double    epsilon  = 1./64.;
	double    delta    = 0.25;
	double    phi      = 0.05;
	uint32_t  m        = UINT32_MAX;

	/* getopt */
	int option_index = 0;
	static int flag  = 0;
	static const char *optstring = "1:2:e:d:p:m:f:h:w:i";
	static const struct option long_options[] = {
		{"min",            no_argument, &flag,     MIN },
		{"median",         no_argument, &flag,  MEDIAN },
		{"const",          no_argument, &flag,   CONST },
		{"epsilon",  required_argument,     0,      'e'},
		{"delta",    required_argument,     0,      'd'},
		{"phi",      required_argument,     0,      'p'},
		{"universe", required_argument,     0,      'm'},
		{"file",     required_argument,     0,      'f'},
		{"info",           no_argument,     0,      'i'},
        {"seed1",    required_argument,     0,      '1'},
        {"seed2",    required_argument,     0,      '2'},
        {"width",    required_argument,     0,      'w'},
        {"height",   required_argument,     0,      'h'},
	};

	NUST_t nust;
	memset(nust.flags, '\0', 5);
	memset(nust.sourceIP, '\0', 16);
	memset(nust.destinationIP, '\0', 16);
	memset(nust.type, '\0', 3);

	DARPA_t darpa;
	memset(darpa.date, '\0', 11);
	memset(darpa.time, '\0', 9);
	memset(darpa.duration, '\0', 9);
	memset(darpa.serv, '\0', 256);
	memset(darpa.sourcePort, '\0', 6);
	memset(darpa.destinationPort, '\0', 6);
	memset(darpa.sourceIP, '\0', 16);
	memset(darpa.destinationIP, '\0', 16);
	memset(darpa.attack_name, '\0', 256);

	while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
		switch (opt) {
			case 0:
				alg[impl_cnt].impl  = flag;
				alg[impl_cnt].index = option_index;
				impl_cnt++;
				break;
			case 'e':
				epsilon = strtod(optarg, NULL);
				break;
			case 'd':
				delta = strtod(optarg, NULL);
				break;
			case 'm':
				m = strtoll(optarg, NULL, 10);
				break;
			case 'p':
				phi = strtod(optarg, NULL);
				break;
			case 'f':
				filename = strndup(optarg, 256);
				if ((NULL != strstr(filename, "NUST"))) {
					format = NUST;
				}
				if ((NULL != strstr(filename, "DARPA"))) {
					format = DARPA;
				}
				if ((NULL != strstr(filename, "Zipfian"))) {
					format = BINARY;
				}
				if ((NULL != strstr(filename, "Weighted"))) {
					format = BINARY;
				}
				break;
			case '1':
				I1 = strtoll(optarg, NULL, 10);
				break;
			case '2':
				I2 = strtoll(optarg, NULL, 10);
				break;
			case 'w':
				width = strtoll(optarg, NULL, 10);
				break;
			case 'h':
				depth = strtoll(optarg, NULL, 10);
				break;
			case 'i':
			default:
				printusage(argv);
				exit(EXIT_FAILURE);
		}
	}

	if ( NULL == filename ) {
		printusage(argv);
		exit(EXIT_FAILURE);
	}

	if ( epsilon > phi ) {
		free(filename);
		xerror("Epsilon cannot be bigger than phi", __LINE__, __FILE__);
	}

	// This only work since the implementations appear first in long_options
	if ( impl_cnt == 0 ) {
		while (impl_cnt < AMOUNT_OF_IMPLEMENTATIONS) {
			alg[impl_cnt].impl  = long_options[impl_cnt].val;
			alg[impl_cnt].index = impl_cnt;
			impl_cnt++;
		}
	}

	printf("#===========\n");
	printf("#Parameters:\n");
	printf("#===========\n");
	printf("#m:       %"PRIu32"\n", m);
	printf("#delta:   %lf\n", delta);
	printf("#epsilon: %lf\n", epsilon);
	printf("#phi:     %lf\n", phi);
	printf("#seed1:   %"PRIu32"\n", I1);
	printf("#seed2:   %"PRIu32"\n", I2);
	printf("#===========\n\n");

	stream = stream_open(filename);
	stream_set_data_size(stream, 1048576);

	hh_sketch_params_t params_min = {
		.b       = 2,
		.epsilon = epsilon,
		.delta   = delta,
		.m       = m,
		.phi     = phi,
		.f       = &countMin,
	};
	hh_sketch_params_t params_median = {
		.b       = 4,
		.epsilon = epsilon,
		.delta   = delta,
		.m       = m,
		.phi     = phi,
		.f       = &countMedian,
	};

	heavy_hitter_params_t p_min = {
		.hash   = &multiplyShift,
		.params = &params_min,
		.f      = &hh_sketch,
	};
	heavy_hitter_params_t p_median = {
		.hash   = &multiplyShift,
		.params = &params_median,
		.f      = &hh_sketch,
	};
	heavy_hitter_params_t p_const = {
		.hash   = &multiplyShift,
		.params = &params_min,
		.f      = &hh_const_sketch,
	};

	for (k = 0; k < impl_cnt; k++) {
		switch (alg[k].impl) {
			case MIN:
				impl[k] = heavy_hitter_create(&p_min);
				break;
			case MEDIAN:
				impl[k] = heavy_hitter_create(&p_median);
				break;
			case CONST:
				impl[k] = heavy_hitter_create(&p_const);
				break;
			default:
				stream_close(stream);
				free(filename);
				xerror("Unknown heavy hitter implementation.", __LINE__, __FILE__);
		}
	}

#ifdef PRINT
	printf("#Reading Stream");
#endif
	if ( format == BINARY ) {
		i      = 0;
		j      = 0;
		buffer = stream_read(stream);

		// Skip comments
		while ( unlikely(j < stream->data.length && buffer[j] == '#') ) {
			// Read line until newline appears
			while ( likely(i < stream->data.length && buffer[i] != '\n') ) {
				i++;
				// If we run out of buffer we read a new chunk
				if ( unlikely(i == stream->data.length) ) {
					buffer = stream_read(stream);
					i      = 0;
					j      = 0;
				}
			}

			// Goto next character
			i++;

			// If we run out of buffer we read a new chunk
			if ( unlikely(i >= stream->data.length) ) {
				buffer = stream_read(stream);
				i      = 0;
			}

			if ( unlikely(buffer[i] == '\n') ) {
				i++;
				j = i;
				break;
			}

			j = i;
		}

		do {
#ifdef PRINT
			printf(".");
			fflush(stdout);
#endif
			if (!start) {
				buffer = stream_read(stream);
			}

			i = stream->data.length;

			if ( unlikely(buf_size > 0 && j == 0) ) {
				memcpy(&buf[buf_size], &buffer[j], sizeof(uint32_t)-buf_size);
				j += sizeof(uint32_t)-buf_size;

				uid = (uint32_t)((uint8_t)buf[3] << 24)
				    | (uint32_t)((uint8_t)buf[2] << 16)
				    | (uint32_t)((uint8_t)buf[1] << 8 )
					| (uint32_t)((uint8_t)buf[0]);
				for (k = 0; k < impl_cnt; k++) {
					heavy_hitter_update(impl[k], uid, 1);
				}

				buf_size = 0;
			}

			while ( likely(j+sizeof(uint32_t) <= i) ) {
				uid = (uint32_t)((uint8_t)buffer[j+3] << 24)
				    | (uint32_t)((uint8_t)buffer[j+2] << 16)
				    | (uint32_t)((uint8_t)buffer[j+1] << 8 )
					| (uint32_t)((uint8_t)buffer[j]);

				for (k = 0; k < impl_cnt; k++) {
					heavy_hitter_update(impl[k], uid, 1);
				}

				j += sizeof(uint32_t);
			}

			if ( likely(i-j > 0) ) {
				buf_size = i-j;
				memset(buf, '\0', 256);
				memcpy(&buf, &buffer[j], buf_size);
			}

			j      = 0;
			start = false;
		} while ( !stream_eof(stream) );
	} else {
		do {
#ifdef PRINT
			printf(".");
			fflush(stdout);
#endif
			buffer = stream_read(stream);
			i = 0;
			j = 0;

			do {
				// Find end of line
				while ( i < stream->data.length && buffer[i] != '\n') {
					i++;
				}

				if ( unlikely(i >= stream->data.length) ) {
					break;
				}

				// If we have leftovers from last buffer
				if ( buf_size > 0 && j == 0 ) {
					memcpy(&buf[buf_size-1], &buffer[j], i-j);
					string = buf;
					buf_size = 0;
				} else {
					string = &buffer[j];
				}

				// Read NUST format
				switch (format) {
					case NUST:
						c = sscanf(
								string,
	"%lf %"SCNu16" %15s %15s %"SCNu16" %"SCNu16" %4s %"SCNu8" %"SCNu8" %2s %"SCNu32,
								&nust.timestamp,
								&nust.size,
								nust.sourceIP,
								nust.destinationIP,
								&nust.sourcePort,
								&nust.destinationPort,
								nust.flags,
								&nust.protocol,
								&nust.direction,
								nust.type,
								&nust.random
						);

						if ( unlikely(c < 11) ) {
							xerror("Unable to read NUST data", __LINE__, __FILE__);
						}

						#ifndef NDEBUG
						printf("Timestamp: %lf\n", nust.timestamp);
						printf("Size: %"PRIu16"\n", nust.size);
						printf("Source IP: %s\n", nust.sourceIP);
						printf("Source Port: %"PRIu16"\n", nust.sourcePort);
						printf("Destination IP: %s\n", nust.destinationIP);
						printf("Destination Port: %"PRIu16"\n", nust.destinationPort);
						printf("Flags: %s\n", nust.flags);
						printf("Protocol: %"PRIu8"\n", nust.protocol);
						printf("Serv: %"PRIu8"\n", nust.direction);
						printf("Type: %s\n", nust.type);
						printf("Random: %"PRIu32"\n\n", nust.random);
						#endif

						d = sscanf(
								nust.sourceIP,
								"%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8,
								&h1,
								&h2,
								&h3,
								&h4
						);

						if ( unlikely(d < 4) ) {
							xerror("Unable to read source IP", __LINE__, __FILE__);
						}

						uid = (uint32_t)(h1 << 24) | (h2 << 16) | (h3 << 8) | h4;
						for (k = 0; k < impl_cnt; k++) {
							heavy_hitter_update(impl[k], uid, 1);
						}

						break;
					case DARPA:
						c = sscanf(
								string,
							"%"PRIu32" %10s %8s %8s %s %5s %5s %15s %15s %lf %s",
								&darpa.id,
								darpa.date,
								darpa.time,
								darpa.duration,
								darpa.serv,
								darpa.sourcePort,
								darpa.destinationPort,
								darpa.sourceIP,
								darpa.destinationIP,
								&darpa.attack_score,
								darpa.attack_name
						);

						if ( unlikely(c < 11) ) {
							xerror("Unable to read DARPA data", __LINE__, __FILE__);
						}

						#ifndef NDEBUG
						printf("ID: %d\n", darpa.id);
						printf("Date: %s\n", darpa.date);
						printf("Time: %s\n", darpa.time);
						printf("Duration: %s\n", darpa.duration);
						printf("Serv: %s\n", darpa.serv);
						printf("Source Port: %s\n", darpa.sourcePort);
						printf("Destination Port: %s\n", darpa.destinationPort);
						printf("Source IP: %s\n", darpa.sourceIP);
						printf("Destination IP: %s\n", darpa.destinationIP);
						printf("Attack Score: %lf\n", darpa.attack_score);
						printf("Attack Name: %s\n\n", darpa.attack_name);
						#endif

						d = sscanf(
								darpa.sourceIP,
								"%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8,
								&h1,
								&h2,
								&h3,
								&h4
						);

						if ( unlikely(d < 4) ) {
							xerror("Unable to read source IP", __LINE__, __FILE__);
						}

						uid = (uint32_t)(h1 << 24) | (h2 << 16) | (h3 << 8) | h4;
						for (k = 0; k < impl_cnt; k++) {
							heavy_hitter_update(impl[k], uid, 1);
						}

						break;
					default:
						xerror("Invalid format received", __LINE__, __FILE__);
				}

				if ( unlikely( (c == EOF && errno < 0) ) ) {
					xerror(strerror(errno), __LINE__, __FILE__);
				}

				i++;
				j = i;
			} while( 1 );

			if ( likely(buffer[i] != '\n' && !stream_eof(stream)) ) {
				assert(i-j <= 256);

				buf_size = i-j;

				memset(buf, '\0', 256);
				memcpy(&buf, &buffer[j+1], buf_size);
			}

		} while (!stream_eof(stream));
	}

#ifdef PRINT
	printf("\n\nFinding heavy hitters..\n\n");
#endif

	heavy_hitter_t *hitters;

	printf("Implementation,IP-Address,Index\n");
	for (k = 0; k < impl_cnt; k++) {
		hitters = heavy_hitter_query(impl[k]);

		for (i = 0; i < hitters->count; i++) {
			h1 = (hitters->hitters[i] & (0xff << 24)) >> 24;
			h2 = (hitters->hitters[i] & (0xff << 16)) >> 16;
			h3 = (hitters->hitters[i] & (0xff << 8)) >> 8;
			h4 = (hitters->hitters[i] & 0xff);
			printf("%s,%03"PRIu8".%03"PRIu8".%03"PRIu8".%03"PRIu8",%"PRIu32"\n",
				long_options[alg[k].index].name, h1, h2, h3, h4, hitters->hitters[i]);
		}

		heavy_hitter_destroy(impl[k]);
	}
	printf("\n");

	stream_close(stream);
	free(filename);

	return EXIT_SUCCESS;
}