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
 * @file	ccs811.c
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

#include "ccs811.h"

#define BIT_MASK_9(x) ((uint16_t)(0b111111111 & (x)))

// Combine two bytes to form a single 16-bit word
#define BYTES_TO_WORD(x, y)			((uint16_t)(( (uint16_t) (x) << 8 ) | ((uint16_t) (y))))

// Macro for making sure non-binary numbers are coerced to binary
#define CHECK_BIN(x) ((x) == 0 ? 0 : 1)

#define CCS811_HW_ID_CODE			0x81

// BEGIN PRIVATE FUNCTIONS
/**
 * @brief Simple private function t convert byte to status object.
 */
ccs811_status_t byte_to_status(uint8_t status) {

	ccs811_status_t status_ = (ccs811_status_t ) { .app_valid = (status & 0x10)
					>> 4, .fw_mode = (status & 0x80) >> 7, .data_ready = (status
					& 0x08) >> 3, .error = (status & 0x01) };

	return status_;
}

/**
 * @brief Simple private function to extract raw data from two bytes.
 */
ccs811_raw_data_t bytes_to_raw_data(uint8_t high_byte, uint8_t low_byte) {
	// Convert two bytes to 16 bit word for further manipulation
	uint16_t raw = BYTES_TO_WORD(high_byte, low_byte);

	ccs811_raw_data_t raw_data = (ccs811_raw_data_t ) { .current = raw >> 10,
					.voltage = BIT_MASK_9(raw) };

	return raw_data;
}

/**
 * @brief Get the device error code
 * 
 * TODO: figure out whether the status needs to be read first.
 */
ccs811_error ccs811_get_error_id(ccs811_dev_t *dev) {

	uint8_t error_ = 0;

	ccs811_status_t status = ccs811_status(dev);

	if (status.error) {
		dev->read(dev, CCS811_ERROR_ID, &error_, 1);
		error_ |= 0x80;
	}

	return error_;
}

// END PRIVATE FUNCTIONS

ccs811_error ccs811_init(ccs811_dev_t *dev) {

	// According to the documentation, the ID returned should be 0x81
	uint8_t hw_id;
	volatile ccs811_error error_out;

	// Transition the device from boot to application
	ccs811_start_app(dev);

	dev->read(dev, CCS811_HW_ID, &hw_id, 1);

	if (hw_id != CCS811_HW_ID_CODE)
		return -1;

	return ccs811_write_measmode(dev, 1, 0, 0);

}

ccs811_status_t ccs811_status(ccs811_dev_t *dev) {
	uint8_t status = 0;

	dev->read(dev, CCS811_STATUS, &status, 1);

	return byte_to_status(status);
}

ccs811_error ccs811_write_measmode(ccs811_dev_t *dev, uint8_t mode,
		uint8_t interrupt_state, uint8_t interrupt_threshold) {
	/* The measurement mode is defined as follows:
	 * bit 7 - reserved; write 0
	 * bit 6:4 - drive mode as defined in the header file (can be 0-4)
	 * bit 3 - interrupt state 
	 * bit 2 - interrupt interrupt_threshold
	 * bit 1:0 - reserved (will write 0)
	 */
	uint8_t meas_mode = (mode << 4) | (CHECK_BIN(interrupt_state) << 3)
			| (CHECK_BIN(interrupt_threshold) << 2);

	dev->write(dev, CCS811_MEAS_MODE, &meas_mode, 1);

	return ccs811_get_error_id(dev);
}

ccs811_raw_data_t ccs811_read_raw(ccs811_dev_t *dev) {
	ccs811_raw_data_t ret_val;
	uint8_t ret_data[2];

	dev->read(dev, CCS811_RAW_DATA, &ret_data, 2);

	ret_val = bytes_to_raw_data(ret_data[0], ret_data[1]);

	return ret_val;
}

ccs811_error ccs811_write_env_data(ccs811_dev_t *dev, uint8_t percent_humid,
		uint16_t percent_humid_frac, uint8_t temp_25, uint16_t temp_25_frac) {
	// Take the user provided data and shove it all into a 32-bit word for transmission on the bus./
	uint16_t humid_word = (uint16_t) percent_humid << 9|
	BIT_MASK_9(percent_humid_frac);

	uint16_t temp_word = (uint16_t) temp_25 << 9 | BIT_MASK_9(temp_25_frac);

	uint32_t env_word = (uint32_t) humid_word << 16 | (uint32_t) temp_word;

	dev->read(dev, CCS811_ENV_DATA, (uint8_t *) &env_word, 4);

	return ccs811_get_error_id(dev);
}

ccs811_error ccs811_start_app(ccs811_dev_t *dev) {

	dev->write(dev, CCS811_APP_START, NULL, 0);

	return ccs811_get_error_id(dev);
}

ccs811_alg_results_t ccs811_get_data(ccs811_dev_t *dev) {
	volatile ccs811_alg_results_t results;
	uint8_t data[8];

	dev->read(dev, CCS811_ALG_RESULT_DATA, data, 8);

	results = (ccs811_alg_results_t ) { .eCO2 = BYTES_TO_WORD(data[0], data[1]),
					.TVOC = BYTES_TO_WORD(data[2], data[3]), .status =
							byte_to_status(data[4]), .error = data[5],
					.raw_data = bytes_to_raw_data(data[6], data[7]) };
	return results;
}

uint16_t get_firmware_app_version(ccs811_dev_t *dev) {
	uint16_t data = 0;
	dev->read(dev, CCS811_FW_APP_VERSION, (uint8_t *) &data, 2);

	return data;
}
