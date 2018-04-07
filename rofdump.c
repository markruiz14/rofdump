/*
 * rofdump
 *
 * Utility program for reading ROF binary files from Rigol 8xx series lab
 * power supplies. 
 *
 * Copyright 2017 Mark Ruiz (mark@markruiz.com)
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

#define OFFSET_PERIOD 			16
#define OFFSET_CHANNELS_DATA	28

void print_usage()
{

}

int main(int argc, char *argv[])
{
    // Prevent getopt() from writing to stderr
    opterr = 0;

    int c;
    bool output_csv;

    while((c = getopt(argc, argv, "c")) != EOF) {
        switch(c) {
            case 'c':
                output_csv = true;
                break;

            case '?':
                printf("unrecognized option: -%c\n\n", optopt);   
                print_usage();
                exit(EXIT_FAILURE);
        }    
    }

    int fd = open(argv[optind], O_RDONLY, 0);

    if(fd == -1) {
        perror("Cannnot open ROF file");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;

    // Read the first 3 bytes and make sure we're reading an ROF file
    char magic[4];
    bytes_read = read(fd, &magic, sizeof(magic));

    if(strcmp(magic, "ROF") != 0) {
        printf("Error, not an ROF file!\n");
        exit(EXIT_FAILURE);
    }

    // Try and seek to byte offset for period 
    off_t pos = lseek(fd, OFFSET_PERIOD, SEEK_SET);

    if(pos == -1) {
        perror("Cannnot seek ROF file");
        exit(EXIT_FAILURE);
    }

    // Read the period 
    uint32_t period;
    bytes_read = read(fd, &period, sizeof(period));

    if(bytes_read != sizeof(period)) {
        printf("Could not read period from ROF file\n");
        exit(EXIT_FAILURE);
    }

    // Read the number of data points
    uint32_t points;
    bytes_read = read(fd, &points, sizeof(points));

    // Determine the number of channels in the data section
    pos = lseek(fd, 0, SEEK_END);
    int num_channels = (pos - OFFSET_CHANNELS_DATA) / points / 8;

    if(output_csv) {
        printf("Seconds");
        for(int i = 1; i <= num_channels; i++) {
            printf(",CH%i Voltage,CH%i Current", i, i);
        }
        printf("\n");
    }
    else {
        printf("Data points: %i\n", points);	
        printf("Period: %i second(s)\n", period);
        printf("Number of channels: %i\n\n", num_channels);
    }

    // Read the data for each channel at each point
    pos = lseek(fd, OFFSET_CHANNELS_DATA, SEEK_SET);

    int point = 0;

    do {

        if(output_csv) {
            printf("%i", point);
        }
        else {
            printf("%i:\t", point);
        }

        for(int channel = 0; channel < num_channels; channel++) {
            uint32_t voltage, current;

            // Read the recorded voltage
            bytes_read = read(fd, &voltage, sizeof(voltage));

            // Read the recorded current
            bytes_read = read(fd, &current, sizeof(current));

            if(output_csv) {
                printf(",%f,%f", (float)voltage / 10000, (float)current / 10000);
            }
            else {
                printf("%f(V), %f(A)\t", (float)voltage / 10000, (float)current / 10000);
            }
        }

        printf("\n");
        point++;

    } while(bytes_read != 0);	

    close(fd);

    exit(EXIT_SUCCESS);
}

