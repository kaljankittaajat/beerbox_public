/*
 * serial_port.cpp
 *
 *  Created on: 25.8.2021
 *      Author: keijo
 */
#include "LpcUart.h"
#include "serial_port.h"

#ifdef __cplusplus
extern "C" {
#endif

static LpcUart *EspUart;

void serial_init(void *ctx)
{
	LpcPinMap none = {-1, -1}; // unused pin has negative values in it
	LpcPinMap txpin_esp = { 0, 8 }; // transmit pin
	LpcPinMap rxpin_esp = { 1, 6 }; // receive pin
	LpcUartConfig cfg = { LPC_USART2, 115200, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin_esp, rxpin_esp, none, none };

    EspUart = new LpcUart(cfg);

}

void serial_write_buf(void *ctx, const char *buf, int len)
{
	EspUart->write(buf, len);
}

void serial_write_str(void *ctx, const char *s)
{
	EspUart->write(s);
}

int serial_get_char(void *ctx, char *p)
{
	return EspUart->read(*p);
}

int serial_peek(void *ctx)
{
	return EspUart->peek();
}

#ifdef __cplusplus
}
#endif

