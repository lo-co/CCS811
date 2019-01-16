# CCS811 Air Quality Driver

This library provides a platform independent driver for the CCS811 Air Quality 
measurement chip.

## Implementation details

The details of the I2C bus communication are left to the implementation.  In `ccs811.h`, a `struct` describing the IAQ device is defined as follows:

```
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
```

Read and write operations are defined as as  function pointers with type of `ccs811_func_ptr` which looks like:

```
typedef uint8_t (*ccs811_func_ptr)(ccs811_dev_t *dev, uint8_t reg_addr,
		uint8_t *data, uint16_t len);
```

These should be implemented in the code that calls this.  An example of implementation in the STM32 domain using the HAL provided by the manufacturer might looks like this 

```
uint8_t read_iaq_i2c(ccs811_dev_t *dev, uint8_t reg_addr, uint8_t *data,
		uint16_t len) {

	uint8_t result = 0;
	result = HAL_I2C_Master_Transmit(&hi2c1, dev->address, &reg_addr, 1, 100);

	if (result == HAL_OK) {
		result = HAL_I2C_Master_Receive(&hi2c1, dev->address, data, len, 100);
	}
	return hi2c1.ErrorCode;
}
```

for the `read` function where `dev` is defined as something that looks like

```
	ccs811_dev_t iaq_dev = { 
                            .address = (CCS811_DEFAULT_ADDRESS << 1), 
                            .read =	&read_iaq_i2c, 
                            .write = &write_iaq_i2c 
                        };
```

(in the ST HAL, the address is a 7 bit address taht must be left-shifted by 1 bit).  The `write` implementation is left as an exercise for the user.

## Links

* [CCS811 Datasheet](https://cdn-shop.adafruit.com/product-files/3566/3566_datasheet.pdf)
* [Adafruit Breakout Board](https://learn.adafruit.com/adafruit-ccs811-air-quality-sensor/)
* [Sparkfun Breakout Board](https://www.sparkfun.com/products/14193)
* [Understanding the I2C Bus](http://www.ti.com/lit/an/slva704/slva704.pdf)