/*
 * LpcUart.cpp
 *
 *  Created on: 4.2.2019
 *      Author: keijo
 */

#include <cstring>
#include "LpcUart.h"


static LpcUart *u0;
static LpcUart *u1;
static LpcUart *u2;

extern "C" {
/**
 * @brief	UART interrupt handler using ring buffers
 * @return	Nothing
 */
void UART0_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	if(u0) u0->isr();
}

void UART1_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	if(u1) u1->isr();
}

void UART2_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	if(u2) u2->isr();
}

}


void LpcUart::isr() {
	Chip_UART_IRQRBHandler(uart, &rxring, &txring);
}

bool LpcUart::init = false;

LpcUart::LpcUart(const LpcUartConfig &cfg) {
	CHIP_SWM_PIN_MOVABLE_T tx;
	CHIP_SWM_PIN_MOVABLE_T rx;
	CHIP_SWM_PIN_MOVABLE_T cts;
	CHIP_SWM_PIN_MOVABLE_T rts;
	bool use_rts = (cfg.rts.port >= 0);
	bool use_cts = (cfg.cts.port >= 0);

	if(!init) {
		init = true;
		/* Before setting up the UART, the global UART clock for USARTS 1-4
		 * must first be setup. This requires setting the UART divider and
		 * the UART base clock rate to 16x the maximum UART rate for all
		 * UARTs.
		 * */
		/* Use main clock rate as base for UART baud rate divider */
		Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);
	}

	uart = nullptr; // set default value before checking which UART to configure

	if(cfg.pUART == LPC_USART0) {
		if(u0) return; // already exists
		else u0 = this;
		tx = SWM_UART0_TXD_O;
		rx = SWM_UART0_RXD_I;
		rts = SWM_UART0_RTS_O;
		cts = SWM_UART0_CTS_I;
		irqn = UART0_IRQn;
	}
	else if(cfg.pUART == LPC_USART1) {
		if(u1) return; // already exists
		else u1 = this;
		tx = SWM_UART1_TXD_O;
		rx = SWM_UART1_RXD_I;
		rts = SWM_UART1_RTS_O;
		cts = SWM_UART1_CTS_I;
		irqn = UART1_IRQn;
	}
	else if(cfg.pUART == LPC_USART2) {
		if(u2) return; // already exists
		else u2 = this;
		tx = SWM_UART2_TXD_O;
		rx = SWM_UART2_RXD_I;
		use_rts = false; // UART2 does not support handshakes
		use_cts = false;
		irqn = UART2_IRQn;
	}
	else {
		return;
	}

	uart = cfg.pUART; // set the actual value after validity checking


	if(cfg.tx.port >= 0) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, cfg.tx.port, cfg.tx.pin, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(tx, cfg.tx.port, cfg.tx.pin);
	}

	if(cfg.rx.port >= 0) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, cfg.rx.port, cfg.rx.pin, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(rx, cfg.rx.port, cfg.rx.pin);
	}

	if(use_cts) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, cfg.cts.port, cfg.cts.pin, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(cts, cfg.cts.port, cfg.cts.pin);
	}

	if(use_rts) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, cfg.rts.port, cfg.rts.pin, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(rts, cfg.rts.port, cfg.rts.pin);
	}


	/* Setup UART */
	Chip_UART_Init(uart);
	Chip_UART_ConfigData(uart, cfg.data);
	Chip_UART_SetBaud(uart, cfg.speed);

	if(use_rts && cfg.rs485) {
		uart->CFG |= (1 << 20); // enable rs485 mode
		//uart->CFG |= (1 << 18); // OE turnaraound time
		uart->CFG |= (1 << 21);// driver enable polarity (active high)
	}

	Chip_UART_Enable(uart);
	Chip_UART_TXEnable(uart);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_RB_SIZE);


	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(uart, UART_INTEN_RXRDY);
	Chip_UART_IntDisable(uart, UART_INTEN_TXRDY);	/* May not be needed */

	/* Enable UART interrupt */
	NVIC_EnableIRQ(irqn);
}

LpcUart::~LpcUart() {
	if(uart != nullptr) {
		NVIC_DisableIRQ(irqn);
		Chip_UART_IntDisable(uart, UART_INTEN_RXRDY);
		Chip_UART_IntDisable(uart, UART_INTEN_TXRDY);

		if(uart == LPC_USART0) {
			u0 = nullptr;
		}
		else if(uart == LPC_USART1) {
			u1 = nullptr;
		}
		else if(uart == LPC_USART2) {
			u2 = nullptr;
		}
	}
}


int  LpcUart::free()
{
	return RingBuffer_GetCount(&txring);;
}

int  LpcUart::peek()
{
	return RingBuffer_GetCount(&rxring);
}

int  LpcUart::read(char &c)
{
	return Chip_UART_ReadRB(uart, &rxring, &c, 1);
}

int  LpcUart::read(char *buffer, int len)
{
	return Chip_UART_ReadRB(uart, &rxring, buffer, len);
}

int LpcUart::write(char c)
{
	return Chip_UART_SendRB(uart, &txring, &c, 1);
}

int LpcUart::write(const char *s)
{
	return Chip_UART_SendRB(uart, &txring, s, strlen(s));
}

int LpcUart::write(const char *buffer, int len)
{
	return Chip_UART_SendRB(uart, &txring, buffer, len);
}

void LpcUart::txbreak(bool brk)
{
	// break handling not implemented yeet
}

bool LpcUart::rxbreak()
{
	// break handling not implemented yeet
	return false;
}

void LpcUart::speed(int bps)
{
	Chip_UART_SetBaud(uart, bps);
}

bool LpcUart::txempty()
{
	return (RingBuffer_GetCount(&txring) == 0);
}
