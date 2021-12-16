/*
 * systick.h
 *
 *  Created on: 6.9.2021
 *      Author: keijo
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_

#ifdef __cplusplus
extern "C" {
#endif

uint32_t get_ticks(void);

#ifdef __cplusplus
}
#endif

void Sleep(int ms);

/* this function is required by the modbus library */
uint32_t millis();


#endif /* SYSTICK_H_ */
