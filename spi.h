/**
 * @file spi.h
 * @author Chesley Kraniak (ckraniak@live.com)
 * @brief Simple interface to Linux SPI bus
 * @version 1.0
 * @date 2021-07-12
 * 
 * @copyright Copyright (c) 2021, Chesley Kraniak, All Rights Reserved
 * 
 * Defined is a simple C interface to the Linux SPI system, and
 * the special function "spi_read_two_bytes()" that enables the
 * core MCP3301 two-byte-read to occure.
 * 
 * The internals will attempt to save and restore the SPI settings
 * upon initialization and destruction. This is not threadsafe, and
 * I do not plan to make it threadsafe. It also only works with one
 * SPI device at a time; if you open a second device, only the
 * configuration of the second SPI device is stored, and closing the
 * first SPI device will cause the first device to be set to the
 * initial settings of the secodn device.
 * 
 * This was written in this way because it was convenient to do so
 * at the time. If you need other behavior, manually editing spi.c
 * should be reasonably straightforward :).
 */

#include <linux/spi/spidev.h>
#include <stdint.h>

/**
 * @brief A minimal group of configuration settings required to initialize a SPI bus
 * 
 * @remarks Refer to https://www.kernel.org/doc/html/latest/spi/spidev.html
 *          for extensive documentation about the SPI bus and its
 *          configuration in Linux.
 */
typedef struct spi_settings {
    /**
     * @brief Configures the SPI bus mode (0, 1, 2, or 3)
     * 
     * @remarks See SPI bus documentation (and the documentation of your chip) to
     *          learn more about what this does.
     */
    uint8_t  mode;

    /**
     * @brief Configures the endian-ness of the physical signal on the bus
     */
    uint8_t  is_lsb_first;

    /**
     * @brief Configures the number of bits in a single SPI "word"
     */
    uint8_t  bits_per_word;

    /**
     * @brief Configures the bus baud rate
     */
    uint32_t max_speed_hz;    
} spi_settings_t;

/**
 * @brief Print nicely-formatted information about the given SPI
 *        settings to STDOUT
 * 
 * @param settings The SPI settings data to print
 */
void print_spi_settings(spi_settings_t *settings);

/**
 * @brief Read the configuration of the given SPI device
 * 
 * @param fd The SPI device file descriptor
 * @param settings The location to write the SPI settings data to
 * @return int 0 for a successful read, nonzero otherwise
 */
int spi_read_settings(int fd, spi_settings_t *settings);

/**
 * @brief Write the settings to the given SPI device
 * 
 * @param fd The SPI device file descriptor
 * @param settings The SPI settings to write to the device
 * @return int 0 for a successful read, nonzero otherwise
 */
int spi_write_settings(int fd, spi_settings_t *settings);

/**
 * @brief Print nicely-formatted information about the given SPI device's
 *        settings to STDOUT
 * 
 * @param fd The SPI device file descriptor
 */
void print_current_spi_settings(int fd);

/**
 * @brief Initializes and configures the named SPI device
 * 
 * @param device_name The name of the SPI device file to open (e.g. "/dev/spidev0.0")
 * @param settings The SPI settings to initialize the device to
 * @return int The SPI device file descriptor
 */
int spi_init(const char *device_name, spi_settings_t *settings);

/**
 * @brief Shut down the SPI system and close the connection to the given SPI device
 * 
 * @param fd The SPI device file descriptor
 */
void spi_shutdown(int fd);

/**
 * @brief Reads two bytes of data from the given SPI device
 * 
 * @param fd The SPI device file descriptor
 * @param out The memory buffer to write the data to
 * @return int The number of bytes read, else 0 for for EOF or -1 for errors
 */
int spi_read_two_bytes(int fd, uint8_t* out);
