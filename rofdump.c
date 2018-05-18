/*
 * rofdump
 *
 * Utility program for reading ROF binary files from Rigol 8xx series lab
 * power supplies. 
 *
 * Copyright (C) 2017 Mark Ruiz (mark@markruiz.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the standard MIT license.  See COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#ifdef __MACH__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#define VERSION                 "1.0"
#define OFFSET_PERIOD           16
#define OFFSET_CHANNELS_DATA    28

void print_usage(int error)
{
	FILE *output = error ? stderr : stdout;
	fprintf(output, "ROFDUMP %s: A utility for reading ROF files produced"
			"by Rigol 8xx lab power supplies - Mark Ruiz (mark@markruiz.com)"
			"\n", VERSION);
	fprintf(output, "usage: rofdump [-h] [-c] filename\n");
	fprintf(output, "options:\n");
	fprintf(output, "\t-h\tprint usage\n");
	fprintf(output, "\t-c\toutput CSV\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage(1);
		exit(EXIT_FAILURE);
	}

	// Prevent getopt() from writing to stderr
	opterr = 0;

	int ch;
	bool output_csv;

	while ((ch = getopt(argc, argv, "ch")) != EOF) {
		switch(ch) {
		case 'c':
			output_csv = true;
			break;

		case 'h':
			print_usage(0);
			exit(EXIT_SUCCESS);

		case '?':
			printf("rofdump: invalid option '-%c'\n", optopt);   
			print_usage(1);
			exit(EXIT_FAILURE);
		}    
	}

	int fd = open(argv[optind], O_RDONLY, 0);

	if (fd == -1) {
		perror("Cannnot open specified file");
		exit(EXIT_FAILURE);
	}

	// Read the first 4 bytes and make sure we're reading an ROF file
	char magic[4];
	ssize_t bytes_read = read(fd, &magic, sizeof(magic));

	if (strcmp(magic, "ROF") != 0) {
		printf("Specified file is not a valid ROF file\n");
		exit(EXIT_FAILURE);
	}

	// Try and seek to byte offset for period 
	off_t pos = lseek(fd, OFFSET_PERIOD, SEEK_SET);

	if (pos == -1) {
		perror("Cannnot seek ROF file");
		exit(EXIT_FAILURE);
	}

	// Read the period 
	uint32_t period;
	bytes_read = read(fd, &period, sizeof(period));

	if (bytes_read != sizeof(period)) {
		printf("Could not read period from ROF file\n");
		exit(EXIT_FAILURE);
	}

	// Read the number of data points
	uint32_t points;
	bytes_read = read(fd, &points, sizeof(points));

	if (bytes_read != sizeof(points)) {
		printf("Could not read number of data points from ROF file\n");
		exit(EXIT_FAILURE);
	}

	// Determine the number of channels in the data section
	pos = lseek(fd, 0, SEEK_END);

	if (pos == -1) {
		perror("Could not determine number of channels in ROF file\n");
		exit(EXIT_FAILURE);
	}

	int num_channels = (pos - OFFSET_CHANNELS_DATA) / points / 8;

	if (output_csv) {
		printf("Seconds");
		for (int i = 1; i <= num_channels; i++) 
			printf(",CH%i Voltage,CH%i Current", i, i);
		printf("\n");
	} else {
		printf("Data points: %i\n", points);    
		printf("Period: %i second(s)\n", period);
		printf("Number of channels: %i\n\n", num_channels);
	}

	// Read the data for each channel at each point
	pos = lseek(fd, OFFSET_CHANNELS_DATA, SEEK_SET);

	if (pos == -1) {
		perror("Could not seek to data section of ROF file\n");
		exit(EXIT_FAILURE);
	}

	int seconds = 0;
	off_t end = lseek(fd, 0, SEEK_END);
	lseek(fd, pos, SEEK_SET);

	while (lseek(fd, 0, SEEK_CUR) != end) {
		if (output_csv) 
			printf("%i", seconds);
		else 
			printf("%i:\t", seconds);

		for (int channel = 0; channel < num_channels; channel++) {
			uint32_t value;

			// Read the recorded voltage
			bytes_read = read(fd, &value, sizeof(value));

			if (bytes_read != sizeof(value)) {
				fprintf(stderr, "Failed to read voltage value at %i seconds", 
						seconds);
				exit(EXIT_FAILURE);
			}

			float voltage = (float)value / 10000;

			// Read the recorded current
			bytes_read = read(fd, &value, sizeof(value));

			if (bytes_read != sizeof(value)) {
				fprintf(stderr, "Failed to read current value at %i seconds", 
						seconds);
				exit(EXIT_FAILURE);
			}

			float current = (float)value / 10000;

			if (output_csv) 
				printf(",%f,%f", voltage, current);
			else 
				printf("%f(V), %f(A)\t", voltage, current);
		}

		printf("\n");
		seconds += period;
	} 

	exit(EXIT_SUCCESS);
}

