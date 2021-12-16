/*
 * @brief Common SystemInit function for LPC15xx chips
 *
 * @note
 * Copyright 2013-2014, 2019, 2020 NXP
 * All rights reserved.
 *
 * @par
 * NXP Confidential. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms.
 * 
 * By expressly accepting such terms or by downloading, installing, activating
 * and/or otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.
 * 
 * If you do not agree to be bound by the applicable license terms, then you may not
 * retain, install, activate or otherwise use the software.
 */

 #if defined(NO_BOARD_LIB)
 #include "chip.h"
 #else
 #include "board.h"
 #endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

#if defined(NO_BOARD_LIB)
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;
#endif

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Set up and initialize hardware prior to call to main */
void SystemInit(void)
{
#if defined(NO_BOARD_LIB)
	/* Chip specific SystemInit */
	Chip_SystemInit();
#else
	/* Board specific SystemInit */
	Board_SystemInit();
#endif
}
