/*
 * ModbusRegister.h
 *
 *  Created on: 13.2.2019
 *      Author: keijo
 */

#ifndef MODBUSREGISTER_H_
#define MODBUSREGISTER_H_

#include "ModbusMaster.h"

class ModbusRegister {
public:
	ModbusRegister(ModbusMaster *master, int address);
	ModbusRegister(const ModbusRegister &)  = delete;
	virtual ~ModbusRegister();
	operator int();
	const ModbusRegister &operator=(int value);
private:
	ModbusMaster *m;
	int addr;
};

#endif /* MODBUSREGISTER_H_ */
