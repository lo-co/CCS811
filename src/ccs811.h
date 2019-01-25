/*
 * Copyright (c) 2018 - MSR Consulting LLC
 *
 * This file is part of CCS811_Driver.
 *
 * CCS811_Driver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CCS811_Driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CCS811_Driver.  If not, see <https://www.gnu.org/licenses/>.
 *
 * @file	ccs811.h
 *
 * @brief Driver for the CCS811 Ultra-Low Power Digital Gas Sensor for Monitoring Indoor Air Quality
 *
 *
 *
 *
 * @date	15 January 2019
 * @version 0.1.0
 * @author	Matt Richardson
 */

#ifndef CCS811_H_
#define CCS811_H_

#include <stdint.h>

#define NULL 						((void *)0)

// Default address of the device is 0x5B
#define CCS811_DEFAULT_ADDRESS		0x5A

// BEGIN REGISTER MAP
// APPLICATION REGISTER
#define CCS811_STATUS				0x00
#define CCS811_MEAS_MODE 			0x01
#define CCS811_ALG_RESULT_DATA		0x02
#define CCS811_RAW_DATA				0x03
#define CCS811_ENV_DATA				0x05
#define CCS811_NTC					0x06
#define CCS811_THRESHOLDS			0x10
#define CCS811_BASELINE				0x11
#define CCS811_HW_ID				0x20
#define CCS811_HW_VERSION			0x21
#define CCS811_FW_BOOT_VERSION		0x23
#define CCS811_FW_APP_VERSION		0x24
#define CCS811_ERROR_ID				0xE0
#define CCS811_SW_RESET				0xFF

// BOOT LOADER REGISTER
#define CCS811_APP_ERASE			0xF1
#define CCS811_APP_DATA				0xF2
#define CCS811_APP_VERIFY			0xF3
#define CCS811_APP_START			0xF4

//END REGISTER MAP

// ERROR DEFINITIONS
#define CCS811_WRITE_REG_INVALID	0x00
#define	CCS811_READ_REG_INVALID 	0x01
#define CCS811_MEASMODE_INVALID		0x02
#define CCS811_MAX_RESISTANCE 		0x04
#define CCS811_HEATER_FAULT 		0x08
#define CCS811_HEATER_SUPPLY		0x10

// MEASUREMENT MODES
#define CCS811_IDLE					0x00
#define CCS811_CONSTANT_POWER_1s	0x01
#define CCS811_PULSE_HEATING		0x02
#define CCS811_LOW_POWER			0x03
#define CCS811_CONSTANT_POWER_250ms	0x04

/*! Forward declared device structure for use in function pointer */
typedef struct ccs811_dev_s ccs811_dev_t;

/*! Function pointer for implementation specific details of I2C IO */
typedef uint8_t (*ccs811_func_ptr)(ccs811_dev_t *dev, uint8_t reg_addr,
		uint8_t *data, uint16_t len);

/*! Error number for the CS811.  If the MSB is set, then there is an error */
typedef uint8_t ccs811_error;

/**
 * @brief CCS811 device structure
 */
struct ccs811_dev_s {
	/*! Device address; default is 0x5B */
	uint8_t address;
	/*! Harware ID */
	uint8_t hw_id;
	/*! Harware version */
	uint8_t hw_version;
	/*! Firmware version of the boot code */
	uint16_t fw_boot_version;
	/*! Firmware version of the application code */
	uint16_t fw_app_version;
	/*! Read function pointer for the I2C implementation */
	ccs811_func_ptr read;
	/*! Write function pointer for the I2C implementation */
	ccs811_func_ptr write;
};

/**
 * @brief Status byte returned via CCS811_STATUS
 * 
 * TODO: figure out if the error flag is cleared on reading.
 */
typedef struct ccs811_status_s {
	/*! 0 = Firmware boot mode; 1 = Firmware in application mode */
	uint8_t fw_mode;
	/*! 0 = no application firmware loaded; 1 = valid application firmware loaded */
	uint8_t app_valid;
	/*! 0 = No new data available; 1 = new data sample ready call CCS811_ALG_RESULT_DATA */
	uint8_t data_ready;
	/*! 0 = no error; 1 = error on sensor available in CCS811_ERROR_ID register */
	uint8_t error;
} ccs811_status_t;

/**
 * @brief Raw data from the sense resistor.
 */
typedef struct ccs811_raw_data_s {
	/*! Current through the sensor - 0 -63 uA*/
	uint8_t current;
	/*! Voltage across sensor with selected current */
	uint16_t voltage;
} ccs811_raw_data_t;

/**
 * Results from the multibyte read-only register 0x02.
 * If only the eCO2 is required, read only the first two bytes.
 * If TVOC is required, read first 4 bytes.
 *
 */
typedef struct ccs811_alg_results_s {
	/*! Equivalent carbon dioxide in ppm; valid from 400 to 8192 ppm */
	uint16_t eCO2;
	/*! Total volatile organic carbon in ppb; valid from 0 to 1187 ppb */
	uint16_t TVOC;
	ccs811_status_t status;
	uint8_t error;
	ccs811_raw_data_t raw_data;

} ccs811_alg_results_t;

/**
 * @brief Data from NTC resistor from which ambient temperature can be determined.
 */
typedef struct ccs811_ntc_data_s {
	/*! Voltage across reference resistor */
	uint16_t Vref;
	/*! Voltage across NTC resistor*/
	uint16_t VNTC;
} ccs811_ntc_data_t;

/**
 * @brief Initialize the IAQ measurement device.
 *
 * This will initialize the CCS811 device as follows:
 * * Verify correct response to a request for the hardware ID
 * * Start the application
 * * Retrieve the status and return an error if error is present.
 * * Set the measurement mode to 1 s at constant power
 *
 * @param dev Definition of the device.  Should be properly populated
 * with the functions pointers for read and write functionality and the
 * device address.
 *
 * @return Error status.
 */
ccs811_error ccs811_init(ccs811_dev_t *dev);

/**
 * @brief Get the current status of the CCS811 device.
 *
 * The status register is a single byte and indicates whether the device is
 * active, there is new data and if there is an error.  The status *will not*
 * return the error id, just whether there is an error or not.
 *
 * @param dev Definition of the device.  Should be properly populated
 * with the functions pointers for read and write functionality and the
 * device address.
 *
 * @return A status object containing the parsed information from the status byte.
 *
 *
 */
ccs811_status_t ccs811_status(ccs811_dev_t *dev);

/**
 * @brief Enable sensor drive modes and interrupt_state
 *
 * @param dev Definition of the device.  Should be properly populated
 * with the functions pointers for read and write functionality and the
 * device address.
 * @param mode byte that can be set to 0-4 as defined below
 * @param interrupt_state 0 = interrupt disabled; 1 = interrupt driven low when new sample available
 * @param interrupt_threshold 0 = interrupt mode operates normally; 1 = interrupt mode asserts if 
 * `CCS811_ALG_RESULT_DATA` crosses threshold
 * 
 * @return Returns any errors.
 * 
 * 
 */
ccs811_error ccs811_write_measmode(ccs811_dev_t *dev, uint8_t mode,
		uint8_t interrupt_state, uint8_t interrupt_threshold);

/**
 * @brief Returns raw sensor data.
 * 
 * Raw data is returned as 2 bytes containing the latest readings from the sense resistor.
 * The 6 most significant bits of the first byte contain the sensor amperage ranging from 
 * 0-63 uA.  The lower 10 bits of the 16 bit word contain the voltage acros the sensor with 
 * the selected current. 
 *
 * @param dev CCS811 device structure.
 * 
 * @return A raw data opbject containing the current and the voltage across the sensor
 */
ccs811_raw_data_t ccs811_read_raw(ccs811_dev_t *dev);

/**
 * @brief Write environmental data for compensation.
 *
 * @param dev 					Device information for writing to the bus.
 * @param percent_humid 		Uses first 7 bits of number to represent whole number humidity
 * @param percent_humid_frac 	9 bits representing the fraction of humidity percentage (in 1/512 increments)
 * @param temp_25				7 bits representing the whole number of temperature with an offset of 25 degrees C
 * @param temp_25_frac			9 bits representing the fraction of temperature in 1/512 increments.
 *
 */
ccs811_error ccs811_write_env_data(ccs811_dev_t *dev, uint8_t percent_humid,
		uint16_t percent_humid_frac, uint8_t temp_25, uint16_t temp_25_frac);

/**
 * @brief Set the bootloader register to indicate an application start.
 *
 * Used to transition the CCS811 state from boot to application mode, a write with no data is
 * required. Before performing a write to APP_START the Status register should be accessed to
 * check if there is a valid application present.  THIS FUNCTION MUST BE CALLED FIRST!!!
 *
 * @param dev CCS811 device structure.
 *
 * @return Error code.
 */
ccs811_error ccs811_start_app(ccs811_dev_t *dev);

/**
 * @brief Retrieve the core data set defined by the structure `ccs811_alg_results_t`.
 * 
 * This performs a multibyte read on the bus and will retrieve *all 8 bytes* for the 
 * data structure returned.  These bytes are ordered as follows:
 * 
 * | Byte | Data |
 * | ---- | ---- |
 * | 0-1  | Equivalent carbon dioxide |
 * | 2-3  | Total volatile organic carbon |
 * | 4    | Status |
 * | 5    | Error ID |
 * | 6-7  | Raw Data | 
 * 
 */
ccs811_alg_results_t ccs811_get_data(ccs811_dev_t *dev);

ccs811_error ccs811_get_error_id(ccs811_dev_t *dev);

uint16_t get_firmware_app_version(ccs811_dev_t *dev);

#endif /* CCS811_H_ */
