/*
 * SerialPort.h
 *
 *  Created on: 10.2.2016
 *      Author: krl
 *  Adapter class to provide Arduino Serial compatible interface
 */

#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include "LpcUart.h"

class SerialPort {
public:
	SerialPort();
	virtual ~SerialPort();
	int available();
	void begin(int speed = 9600);
	int read();
	int write(const char* buf, int len);
	int print(int val, int format);
	void flush();
private:
	LpcUart *u;
};

#endif /* SERIALPORT_H_ */
