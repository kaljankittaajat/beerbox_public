/**
@file
Utility Functions for Manipulating Words

@defgroup util_word "util/word.h": Utility Functions for Manipulating Words
@code#include "util/word.h"@endcode

This header file provides utility functions for manipulating words.

*/
/*

  word.h - Utility Functions for Manipulating Words

  This file is part of ModbusMaster.

  ModbusMaster is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ModbusMaster is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ModbusMaster.  If not, see <http://www.gnu.org/licenses/>.

  Written by Doc Walker (Rx)
  Copyright © 2009-2015 Doc Walker <4-20ma at wvfans dot net>

*/


#ifndef _UTIL_WORD_H_
#define _UTIL_WORD_H_


/** @ingroup util_word
    Return low word of a 32-bit integer.

    @param uint32_t ww (0x00000000..0xFFFFFFFF)
    @return low word of input (0x0000..0xFFFF)
*/
static inline uint16_t lowWord(uint32_t ww)
{
  return (uint16_t) ((ww) & 0xFFFF);
}


/** @ingroup util_word
    Return high word of a 32-bit integer.

    @param uint32_t ww (0x00000000..0xFFFFFFFF)
    @return high word of input (0x0000..0xFFFF)
*/
static inline uint16_t highWord(uint32_t ww)
{
  return (uint16_t) ((ww) >> 16);
}

/* utility functions for porting ModbusMaster to LPCXpresso
 * added by krl
 */
static inline uint16_t word(uint8_t ww)
{
  return (uint16_t) (ww);
}

static inline uint16_t word(uint8_t h, uint8_t l)
{
  return (uint16_t) ((h << 8) | l);
}

static inline uint8_t highByte(uint16_t v)
{
  return (uint8_t) ((v >> 8) & 0xFF);
}

static inline uint16_t lowByte(uint16_t v)
{
	return (uint8_t) (v & 0xFF);
}

static inline uint8_t bitRead(uint8_t v, uint8_t n)
{
	return (uint8_t) (v & (1 << n) ? 1 : 0);
}

static inline void bitWrite(uint16_t& v, uint8_t n, uint8_t b)
{
	if(b) v = v | (1 << n);
	else v = v & ~(1 << n);
}


#endif /* _UTIL_WORD_H_ */
