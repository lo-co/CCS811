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

// BEGIN PRIVATE FUNCTIONS
/**
 * @brief Simple private function t convert byte to status object.
 */
ccs811_status_t byte_to_status(uint8_t status)
{
	uint8_t status = 0;

	ccs811_status_t status_ = (ccs811_status_t){.app_valid = (status & 0x10) >> 4, .fw_mode = (status & 0x80) >> 7, .data_ready = (status & 0x08) >> 3, .error = (status & 0x01)};

	return status_;
}

/**
 * @brief Simple private function to extract raw data from two bytes.
 */
ccs811_raw_data_t bytes_to_raw_data(uint8_t high_byte, uint8_t low_byte)
{
	// Convert two bytes to 16 bit word for further manipulation
	uint16_t raw = BYTES_TO_WORD(high_byte, low_byte);

	ccs811_raw_data_t raw_data = (ccs811_raw_data_t){
		.current = raw >> 10,
		.voltage = BIT_MASK_9(raw)};

	return raw_data;
}

// END PRIVATE FUNCTIONS

uint8_t ccs811_init(ccs811_dev_t *dev)
{

	// According to the documentation, the ID returned should be 0x81
	uint8_t hw_id;

	// Transition the device from boot to application
	ccs811_start_app(dev);

	dev->read(dev, CCS811_HW_ID, &hw_id, 1);

	if (hw_id != CCS811_HW_ID_CODE)
		return -1;

	ccs811_status_t status = ccs811_status(dev);

	if (status.error != 0)
	{
		return status.error;
	}
	// Set the mode to poll every second at constant power
	uint8_t mode = 0x01 << 4;
	dev->write(dev, CCS811_MEAS_MODE, &mode, 1);

	status = ccs811_status(dev);

	return status.error;
}

ccs811_status_t ccs811_status(ccs811_dev_t *dev)
{
	uint8_t status = 0;

	dev->read(dev, CCS811_STATUS, &status, 1);

	return byte_to_status(status);
}

ccs811_raw_data_t ccs811_read_raw(ccs811_dev_t *dev)
{
	ccs811_raw_data_t ret_val;

	return ret_val;
}

uint8_t ccs811_write_env_data(ccs811_dev_t *dev, uint8_t percent_humid,
							  uint16_t percent_humid_frac, uint8_t temp_25, uint16_t temp_25_frac)
{
	// Take the user provided data and shove it all into a 32-bit word for transmission on the bus./
	uint16_t humid_word = (uint16_t)percent_humid
							  << 9 |
						  BIT_MASK_9(percent_humid_frac);

	uint16_t temp_word = (uint16_t)temp_25 << 9 | BIT_MASK_9(temp_25_frac);

	uint32_t env_word = (uint32_t)humid_word << 16 | (uint32_t)temp_word;

	dev->write(dev, CCS811_ENV_DATA, (uint8_t *)&env_word, 4);

	return 0;
}

uint8_t ccs811_start_app(ccs811_dev_t *dev)
{

	return dev->write(dev, CCS811_APP_START, NULL, 1);
}

ccs811_alg_results_t ccs811_get_data(ccs811_dev_t *dev)
{
	ccs811_alg_results_t results;
	uint8_t data[8];

	dev->read(dev, CCS811_ALG_RESULT_DATA, data, 8);

	results =
		(ccs811_alg_results_t){.eCO2 = BYTES_TO_WORD(data[0],
													 data[1]),
							   .TVOC = BYTES_TO_WORD(data[2], data[3]),
							   .status = byte_to_status(data[4]),
							   .error = data[5],
							   .raw_data =
								   bytes_to_raw_data(data[6], data[7])};
	return results;
}
