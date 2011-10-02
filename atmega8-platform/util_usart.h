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

#ifndef UTIL_USART_H
#define UTIL_USART_H

#include "defines.h"


/* USART handling shit  */


void usart_process();

void handlePingCommand();
void handleRxCommand();
void handleResetCommand();
void handleMotorErrorCommand();
void handleRxStartPosCommand();
void handleCopterStateCommand();
void handleMotorCommand();
void handleRxControlCommand();
void handleGyroCommand();
void handleGyroControlCommand();
void handlePanicCommand();
void handleGyroCompensationCommand();
void handleAccCompensationCommand();
void handleAccCommand();

void setup_USART();
char USART_recv_byte();
void USART_send_byte(char);


#endif

