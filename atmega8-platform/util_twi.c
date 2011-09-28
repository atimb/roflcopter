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

#include <util/twi.h>
#include "util_twi.h"


#define TWI_CLOCK 100000UL



void i2c_init(void)
{
  TWSR = 0;
  TWBR = ((SYSCLK/TWI_CLOCK)-16)/2; 
}

void i2c_start(void) 
{
    TWCR = (1<<TWSTA) | (1<<TWEN) | (1<<TWINT) | (1<<TWIE);
}

void i2c_stop(void)
{
    TWCR = (1<<TWEN) | (1<<TWSTO) | (1<<TWINT);
}

void i2c_write_byte(uint8_t byte)
{ 
    TWSR = 0x00;
    TWDR = byte;
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
}

/*    Receive byte and send ACK         */
void I2C_ReceiveByte(void)
{
   TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE) | (1<<TWEA);
}

/* I2C receive last byte and send no ACK*/
void I2C_ReceiveLastByte(void)
{
   TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
}


volatile unsigned char twi_state = 0;
unsigned char twi_motor_index = 0;

unsigned char motorread = 0,MissingMotor = 0;
unsigned char motor_rx[16],motor_rx2[16];
unsigned char MotorPresent[MAX_MOTORS];
unsigned char MotorError[MAX_MOTORS];
unsigned int I2CError = 0;


void i2c_reset(void)
{
     i2c_stop();
     twi_state = 0;
     twi_motor_index = TWDR;
     twi_motor_index = 0;
     TWCR = 0x80;
     TWAMR = 0;
     TWAR = 0;
     TWDR = 0;
     TWSR = 0;
     TWBR = 0;
     i2c_init();
     i2c_start();
     i2c_write_byte(0);
}



ISR(TWI_vect)
{
 	 static unsigned char missing_motor;
     switch(twi_state++)
        {

		// Writing the Data
        case 0:
                if(++twi_motor_index == 4)  // writing finished -> now read 
                { 
                	twi_motor_index = 0; 
                	twi_state = 3; 
                	i2c_write_byte(0x53+(twi_motor_index*2));
                } 
                else {
					i2c_write_byte(0x52+(twi_motor_index*2));
				}
                break;
        case 1:
                i2c_write_byte(controls.esc_status[twi_motor_index-1]);
                break;
        case 2:
                if(TWSR == 0x30) {
              		if(++MotorError[twi_motor_index-1] == 0) MotorError[twi_motor_index-1] = 255;
             	}
				i2c_stop();
                I2CTimeout = 10;
                twi_state = 0;
                i2c_start();
                break; 

		// Reading Data
        case 3:
               //Transmit 1st byte for reading
                if(TWSR != 0x40)  // Error?
                 { 
                  MotorPresent[motorread] = 0;
                  motorread++;
                  if(motorread >= MAX_MOTORS) motorread = 0;
                  i2c_stop();
                  twi_state = 0;
                 }
                 else 
                 {
                  MotorPresent[motorread] = ('1' - '-') + motorread;
                  I2C_ReceiveByte();
                 }
                MissingMotor = missing_motor;
                missing_motor = 0;
                break;
        case 4: //Read 1st byte and transmit 2nd Byte
                motor_rx[motorread] = TWDR;
                I2C_ReceiveLastByte(); //nack
                break;
        case 5:
                //Read 2nd byte
                motor_rx2[motorread++] = TWDR;
                if(motorread >= MAX_MOTORS) motorread = 0;
                i2c_stop();
                twi_state = 0;
                break;
                
		// Stop things        
        case 6: 
                i2c_stop();               
                I2CTimeout = 10;
                twi_state = 0;
                break;
        default:
			 	twi_state = 0;
                break;        
    }
 	TWCR |= 0x80;
}

