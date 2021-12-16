/*
 * LpcUart.h
 *
 *  Created on: 4.2.2019
 *      Author: keijo
 */

#ifndef LPCUART_H_
#define LPCUART_H_

#include "chip.h"

struct LpcPinMap {
	int port; /* set to -1 to indicate unused pin */
	int pin;  /* set to -1 to indicate unused pin  */
};

struct LpcUartConfig {
	LPC_USART_T *pUART;
	uint32_t speed;
	uint32_t data;
	bool rs485;
	LpcPinMap tx;
	LpcPinMap rx;
	LpcPinMap rts; /* used as output enable if RS-485 mode is enabled */
	LpcPinMap cts;
};


class LpcUart {
public:
	LpcUart(const LpcUartConfig &cfg);
	LpcUart(const LpcUart &) = delete;
	virtual ~LpcUart();
	int  free(); /* get amount of free space in transmit buffer */
	int  peek(); /* get number of received characters in receive buffer */
	int  write(char c);
	int  write(const char *s);
	int  write(const char *buffer, int len);
	int  read(char &c); /* get a single character. Returns number of characters read --> returns 0 if no character is available */
	int  read(char *buffer, int len);
	void txbreak(bool brk); /* set break signal on */
	bool rxbreak(); /* check if break is received */
	void speed(int bps); /* change transmission speed */
	bool txempty();

	void isr(); /* ISR handler. This will be called by the HW ISR handler. Do not call from application */
private:
	LPC_USART_T *uart;
	IRQn_Type irqn;
	/* currently we support only fixed size ring buffers */
	static const int UART_RB_SIZE = 256;
	/* Transmit and receive ring buffers */
	RINGBUFF_T txring;
	RINGBUFF_T rxring;
	uint8_t rxbuff[UART_RB_SIZE];
	uint8_t txbuff[UART_RB_SIZE];
	static bool init; /* set when first UART is initialized. We have a global clock setting for all UARTSs */
};

#endif /* LPCUART_H_ */
