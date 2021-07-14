/**
 * @file main.c
 * @author Chesley Kraniak (ckraniak@live.com)
 * @brief MCP3301 ADC Readings Dumper
 * @version 1.0
 * @date 2021-07-07
 * 
 * @copyright Copyright (c) 2021, Chesley Kraniak, All Rights Reserved
 * 
 * This program is part of a project to construct an always-on
 * scale / perch for falconry. This component resides on a 
 * Raspberry Pi which is built into the perch and is connected
 * to a strain gauge via an instrumentation amplifier and an ADC.
 * 
 * Refer to the MCP3301 datasheet for further information.
 * 
 * There are a very large number of oversights that a more bulletproof
 * embedded program should not have. Error checking is extremely minimal,
 * overflows are not guarded against, timing isn't really being addressed,
 * and a number of other changes could be made to improve the program.
 * Bear in mind that this supported an at-home for-fun project, not an
 * enterprise-grade product. :)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "spi.h"

////////////////////////////////////////////////////////
/// MCP 3301 measurement routines

/**
 * @brief The compile-time-specified SPI device to connect to
 * 
 * @remarks This should be adjusted in source code if the MCP3301 is
 *          connected to a different SPI port.
 */
static const char* device  = "/dev/spidev0.0";

/**
 * @brief SPI settings for the MCP3301
 */
spi_settings_t spi_settings_desired = {
    .mode          = SPI_MODE_3,
    .is_lsb_first  = 0,
    .bits_per_word = 8,
    .max_speed_hz  = 25000
};

/**
 * @brief Reads a single MCP3301 output value from the given SPI device
 * 
 * @remarks 0x8001 is, according to the MCP3301 datasheet, not a valid
 *          output for the device.
 * 
 * @param fd The SPI device to read from which the MCP3301 is connected to
 * @return int16_t The reading from the MCP3301, or 0x8001 in the event of an error
 */
int16_t read_mcp3301_single(int fd) {
    uint8_t raw_data[2];
    int16_t data = 0;
    char sign = 0;

    if (2 != spi_read_two_bytes(fd, raw_data)) {
        printf("read_mcp3301_single: failed to read value\n");
        return 0x8001; // Should be an invalid output for the sensor
    }

    data |= ((raw_data[0] & 0x0F) << 8);
    data |= raw_data[1];
    sign = (raw_data[0] & 0x10) ? 1 : 0;
    if (sign) {
        data -= 4096;
    }

    return data;
}

/**
 * @brief Computes the average of the given 16-bit integers
 * 
 * @param vals The values array to compute the average of
 * @param cnt  The length of the values array given
 * @return double The average of the given values
 */
double average16(int16_t *vals, int cnt) {
    double val = 0;
    for(int i = 0; i < cnt; i++) {
        val += vals[i];
    }
    return val / cnt;
}

/**
 * @brief Computes the standard deviation of the given 16-bit integers
 * 
 * @param vals The values array to compute the standard deviation of
 * @param cnt  The length of the values array given
 * @return double The standard deviation of the given values
 */
double stdev16(int16_t *vals, int cnt) {
    double stdev = 0;
    double avg = average16(vals, cnt);
    for(int i = 0; i < cnt; i++) {
        stdev += pow(vals[i] - avg, 2);
    }
    return sqrt(stdev / cnt);
}

/**
 * @brief A single measurement from the MCP3301
 */
typedef struct mcp3301_measurement {
    /**
     * @brief The integer value measured by the sensor
     */
    int16_t int_val;

    /**
     * @brief The timestamp computed when the reading was taken
     */
    double  timestamp;
} mcp3301_measurement_t;

/**
 * @brief Takes a single MCP3301 measurement from the given SPI device
 * 
 * @param fd The SPI device to read from which the MCP3301 is connected to
 * @param time_init The initial time that the timestamp should be computed from
 * @return mcp3301_measurement_t The MCP3301 measurement value
 */
mcp3301_measurement_t read_mcp3301_measurement(int fd, clock_t time_init) {
    mcp3301_measurement_t mt = {0, 0.0};
    mt.int_val = read_mcp3301_single(fd);
    mt.timestamp = ((double)(clock() - time_init)) / CLOCKS_PER_SEC;
    return mt;
}

////////////////////////////////////////////////////////
/// Filter Buffer class

/**
 * @brief A simple circular data buffer which enables simple data filtering
 * 
 * @remarks As this is a C-style class, one should treat all members
 *          as private and only use the given functions to access the
 *          class. In a more thorough design, this would be hidden in
 *          a *.c file for better protection.
 */
typedef struct filter_buffer {
    int    *data;
    size_t  data_len;
    int     location;
} filter_buffer_t;

/**
 * @brief Creates a new filter_buffer_t with the given buffer length
 * 
 * @param len The length of the buffer to allocate
 * @return filter_buffer_t* The newly-created filter_buffer_t
 */
filter_buffer_t *fb_new(size_t len) {
    filter_buffer_t *fb = (filter_buffer_t *) malloc(sizeof(filter_buffer_t));
    fb->data = (int*) malloc(sizeof(int) * len);
    memset(fb->data, 0, sizeof(int) * len);
    fb->data_len = len;
    fb->location = 0;
    return fb;
}

/**
 * @brief Deletes a given filter_buffer_t
 * 
 * @param fb The filter_buffer_t to delete
 */
void fb_del(filter_buffer_t *fb) {
    free(fb->data);
    free(fb);
}

/**
 * @brief [PRIVATE] Increment the filter_buffer_t's location
 * 
 * @remarks The location field tells the filter_buffer_t where in the
 *          data array new data should be written to.
 * 
 * @param fb The filter_buffer_t to increment the location of
 */
void fb_incr_loc(filter_buffer_t *fb) {
    fb->location = (fb->location + 1) % fb->data_len;
}

/**
 * @brief Push a new value into the filter_buffer_t
 * 
 * @remarks The filter_buffer_t's location is incremented in this operation.
 * 
 * @param fb The filter_buffer_t to push a value into
 * @param val The value to push into the filter_buffer_t
 */
void fb_push(filter_buffer_t *fb, int val) {
    fb->data[fb->location] = val;
    fb_incr_loc(fb);
}

/**
 * @brief Computes the average of the filter_buffer_t's data, ignoring 
 *        the largest and smallest vlues
 * 
 * @param fb The filter_buffer_t to compute the average for
 * @return double The average of the data values (sans max and min) in the given filter_buffer_t
 */
double filter_avg(filter_buffer_t *fb) {
    int max = fb->data[0];
    int min = fb->data[0];
    int sum = max;
    int data;
    for(int i = 1; i < fb->data_len; i++) {
        data = fb->data[i];
        if (data > max) {
            max = data;
        }
        if (data < min) {
            min = data;
        }
        sum += data;
    }
    sum = sum - max - min;
    return ((double) sum) / (fb->data_len - 2);
}

////////////////////////////////////////////////////////
/// Entry point

int main(int argc, char** argv) {
    clock_t t_init = clock();
    int spi_fd = 0;
    if (0 >= (spi_fd = spi_init(device, &spi_settings_desired))) {
        printf("main: could not initialize SPI bus\n");
        goto fail;
    }

    filter_buffer_t *fb = fb_new(16);

    int loops = 0;
    double t = 0;
    mcp3301_measurement_t mt = {0, 0.0};

    // Note: the exit logic was originally set to exit after some number
    // of seconds; now, you are expected to ctrl-C out of this program.
    for(;; t = (((double)(clock() - t_init))/ CLOCKS_PER_SEC)) {
        mt = read_mcp3301_measurement(spi_fd, t_init);
        fb_push(fb, mt.int_val);
        printf("%5.6f\t%d\t%4.3f\n", mt.timestamp, mt.int_val, filter_avg(fb));
        loops++;
    }

    // This code is currently never reached.
    printf("Loops: %d\tAvg time: %2.8f\tLoops/sec: %d\n", loops, t / loops, (int)(loops / t));
    fb_del(fb);
    spi_shutdown(spi_fd);
    return EXIT_SUCCESS;

fail:
    printf("Done with errors\n");
    return EXIT_FAILURE;
}

