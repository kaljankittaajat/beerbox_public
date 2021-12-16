/*
 * ModbusRegister.cpp
 *
 *  Created on: 13.2.2019
 *      Author: keijo
 */

#include "ModbusRegister.h"

ModbusRegister::ModbusRegister(ModbusMaster *master, int address):m(master), addr(address) {
	// TODO Auto-generated constructor stub

}

ModbusRegister::~ModbusRegister() {
	// TODO Auto-generated destructor stub
}

ModbusRegister::operator int() {
	uint8_t result = m->readHoldingRegisters(addr, 1);
	// check if we were able to read
	if (result == m->ku8MBSuccess) {
		return m->getResponseBuffer(0);
	}
	return -1;
}

const ModbusRegister &ModbusRegister::operator=(int value)
{
	m->writeSingleRegister(addr, value); // not checking if write succeeds

	return *this;
}
