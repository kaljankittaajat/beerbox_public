/*
 * serial_port.h
 *
 *  Created on: 25.8.2021
 *      Author: keijo
 */

#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif


void serial_init(void *ctx);
void serial_write_buf(void *ctx, const char *buf, int len);
void serial_write_str(void *ctx, const char *s);
int  serial_get_char(void *ctx, char *p);
int  serial_peek(void *ctx);

#ifdef __cplusplus
}
#endif




#endif /* SERIAL_PORT_H_ */
