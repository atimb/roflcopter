/**
 *  RoflCopter quadrotor project
 *  ATMEGA8 program source
 *
 *  (C) 2011 Attila Incze <attila.incze@gmail.com>
 *  http://atimb.me
 *
 *  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 * 
 */

#ifndef UTIL_TWI_H
#define UTIL_TWI_H

#include "defines.h"


/* TWI handling shit  */

void twi_init();
int twi_read_bytes(uint8_t target, uint8_t addr, int len, uint8_t *buf);
int twi_write_bytes(uint8_t target, uint8_t addr, int len, uint8_t *buf);


#endif
