/**
 * @file spi.c
 * @author Chesley Kraniak (ckraniak@live.com)
 * @brief Implementation of simple interface to Linux SPI bus
 * @version 1.0
 * @date 2021-07-13
 * 
 * @copyright Copyright (c) 2021, Chesley Kraniak, All Rights Reserved
 * 
 * See the associated .h file for more documentation about the functions here.
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <assert.h>

#include "spi.h"

// Internal storage of original (i.e. before initialization) SPI bus settings
static spi_settings_t spi_settings_orig = {0, 0, 0, 0};

void print_spi_settings(spi_settings_t *settings) {
    printf("SPI settings (%p):\nmode:\t\t%u\nis_lsb_first:\t%u\n", settings, settings->mode, settings->is_lsb_first);
    printf("bits_per_word:\t%u\nmax_speed_hz:\t%d\n", settings->bits_per_word, settings->max_speed_hz);
}

static int get_spi_mode(int fd, uint8_t *mode) {
    return ioctl(fd, SPI_IOC_RD_MODE, mode);
}

static int set_spi_mode(int fd, uint8_t mode) {
    return ioctl(fd, SPI_IOC_WR_MODE, &mode);
}

static int get_spi_is_lsb_first(int fd, uint8_t *is_lsb_first) {
    return ioctl(fd, SPI_IOC_RD_LSB_FIRST, is_lsb_first);
}

static int set_spi_is_lsb_first(int fd, uint8_t is_lsb_first) {
    return ioctl(fd, SPI_IOC_WR_LSB_FIRST, &is_lsb_first);
}

static int get_spi_bits_per_word(int fd, uint8_t *bits_per_word) {
    return ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, bits_per_word);
}

static int set_spi_bits_per_word(int fd, uint8_t bits_per_word) {
    return ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
}

static int get_spi_max_speed_hz(int fd, uint32_t *max_speed_hz) {
    return ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, max_speed_hz);
}

static int set_spi_max_speed_hz(int fd, uint32_t speed) {
    return ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

int spi_read_settings(int fd, spi_settings_t *settings) {
    assert(settings != NULL);
    int ret = 0;
    if (0 != (ret = get_spi_mode(fd, &(settings->mode)))) {
        printf("Could not read SPI mode: code %d\n", ret);
        return ret;
    } 
    if (0 != (ret = get_spi_is_lsb_first(fd, &(settings->is_lsb_first)))) {
        printf("Could not read SPI mode: code %d\n", ret);
        return ret;
    } 
    if (0 != (ret = get_spi_bits_per_word(fd, &(settings->bits_per_word)))) {
        printf("Could not read SPI mode: code %d\n", ret);
        return ret;
    } 
    if (0 != (ret = get_spi_max_speed_hz(fd, &(settings->max_speed_hz)))) {
        printf("Could not read SPI max speed (hz): code %d\n", ret);
        return ret;
    }
    return 0; 
}

int spi_write_settings(int fd, spi_settings_t *settings) {
    assert(settings != NULL);
    int ret = 0;
    if (0 != (ret = set_spi_mode(fd, settings->mode))) {
        printf("Could not set SPI mode: code %d\n", ret);
        return ret;
    } 
    if (0 != (ret = set_spi_is_lsb_first(fd, settings->is_lsb_first))) {
        printf("Could not set SPI mode: code %d\n", ret);
        return ret;
    } 
    if (0 != (ret = set_spi_bits_per_word(fd, settings->bits_per_word))) {
        printf("Could not set SPI mode: code %d\n", ret);
        return ret;
    } 
    if (0 != (ret = set_spi_max_speed_hz(fd, settings->max_speed_hz))) {
        printf("Could not set SPI max speed (hz): code %d\n", ret);
        return ret;
    }
    return 0; 
}

void print_current_spi_settings(int fd) {
    spi_settings_t s;
    if (0 != spi_read_settings(fd, &s)) {
        printf("print_current_spi_settings: unable to read settings");
        return;
    }
    print_spi_settings(&s);
}

int spi_init(const char *device_name, spi_settings_t *settings) {
    int fd = open(device_name, O_RDWR);
    if (0 > fd) {
        printf("spi_init: open failed with retval %d\n", fd);
        return fd;
    }

    int ret = 0;
    if (0 != (ret = spi_read_settings(fd, &spi_settings_orig))) {
        printf("spi_init: failed to read SPI settings, code %d\n", ret);
        close(fd);
        return -1;
    }
    if (0 != (ret = spi_write_settings(fd, settings))) {
        printf("spi_init: failed to write SPI settings, code %d\n", ret);
        close(fd);
        return -1;
    }

    return fd;
}

void spi_shutdown(int fd) {
    int ret = 0;
    if (0 != (ret = spi_write_settings(fd, &spi_settings_orig))) {
        printf("spi_shutdown: failed to write SPI settings, code %d\n", ret);
    }
    close(fd);
}

int spi_read_two_bytes(int fd, uint8_t* out) {
    return read(fd, out, 2);
}
