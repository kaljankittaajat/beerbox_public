/*
 * I2C.h
 *
 *  Created on: 21.2.2016
 *      Author: krl
 */

#ifndef I2C_H_
#define I2C_H_

#include "chip.h"

struct I2C_config {
	unsigned int device_number;
	unsigned int speed;
	unsigned int clock_divider;
	unsigned int i2c_mode;
	I2C_config(): device_number(0), speed(100000), clock_divider(40), i2c_mode(IOCON_SFI2C_EN) {};
};

class I2C {
public:
	I2C(const I2C_config &cfg);
	virtual ~I2C();
	bool transaction(uint8_t devAddr, uint8_t *txBuffPtr, uint16_t txSize, uint8_t *rxBuffPtr, uint16_t rxSize);
	bool write(uint8_t devAddr, uint8_t *txBuffPtr, uint16_t txSize);
	bool read(uint8_t devAddr, uint8_t *rxBuffPtr, uint16_t rxSize);
private:
	LPC_I2C_T *device;
	static uint32_t I2CM_XferBlocking(LPC_I2C_T *pI2C, I2CM_XFER_T *xfer);
};

#endif /* I2C_H_ */
