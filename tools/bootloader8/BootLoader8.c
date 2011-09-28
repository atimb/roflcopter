/*
    Modified by Attila Incze <attila.incze@gmail.com> to use
    Jenkins Hash (http://en.wikipedia.org/wiki/Jenkins_hash_function)
    to validate the integrity of the transmission.

    1-24-09
    Copyright Spark Fun ElectronicsŠ 2009
    Nathan Seidle
    
    Wireless bootloader for the ATmega168 and XBee Series 1 modules
	
	This is a small (728 byte) serial bootloader designed to be a robust solution for remote reset and wireless
	booloading. It's not extremely fast, but is very hardy.
    
	The remote unit (the AVR usually) broadcasts the non-visible character ASCII(6). It then waits for a response
	over the serial link for the non-visible character ASCII(5). If received, the remote unit enters bootloading mode.
	If the correct character 5 is not received, the remote unit jumps to the beginning of the regular program code.
	
	Bootloading includes checksum calculation, and timeouts. Timeouts is most important because a wireless
	link does not always deliver segments of the serial stream in a deterministic fashion - a good wireless unit
	will buffer all sorts of stuff, making the connection stream irregular in throughput.
	
	This bootloader accepts a pure binary stream (not an intel hex file format). All file parsing is done on the 
	base side (usually a beefy computer with lots of extra processing ability).
	
	Things I learned from testing:
	
	XBee series 2.5 units have their uses, but not here. I beat my head against the wall trying to form a sensible link and failed.
	Ultimately, plugging series 1 in, it worked wonderfully. If you need point-to-point, series 1 is wonderful. If you really
	need true mesh node networking, Series 2.5 is good.
	
	XBee Series 2.5 ships with CTS enabled! That's why the AT commands through hyperterminal were not working. Grr.
	
	To get a Series 2.5 link to work, you must configure device on XBee Explorer as Coordinator, and the device in your arduino board as the end device.
	
	Trying to use Series 2.5 for a good point-to-point link:
	With CTS Enabled, 19200bps, Packetization Timeout at 3 (default), still bit errors, even with 1ms delay between characters
	With CTS Enabled, 19200bps, Packetization Timeout at 0, with 1ms delay between characters helps a lot, but will get character errors if there is RF interferance (units further than a few feet apart)

	With CTS/RTS Enabled, 19200bps, Packet timeout at 0, no delay but with flow control, we have very solid link -> one way!
	while( (PIND & (1<<CTS)) != 0); //Don't send anything to the XBee, it is thinking
	
	You do not seem to need CTS/RTS/DTR to read or program an XBee.

	With XB24-ZB unit, the end device can transmit all it wants, the coordinator seems to die after a few seconds. This
	was the ultimate downfall of the series 2.5 for me. The link would work, but the coordinator would drop off after a few
	seconds? Series 1 did not do this.
	
	All of the following code works exceptionally well with Series 1 "XB24" "XBee 802.15.4" "v10CD" firmware
	
	To configure the XBees, follow "Lady Ada wireless arduino" info

	Series 1 module settings:
	Baud: 19200
	No flow control (CTS is left on as default)
	No change to packetization timeout (default = 3?)
	
	RTS on XBee board goes up and down with the com advanced trick NOT checked, and hardware control turned ON under terminal

	In VB, turn handshaking off. When RTSEnable = True, the RTS pin goes low, resetting the AVR

	Wireless:
	38 seconds to load 14500 code words (most of the space) at 38400 / 8MHz (internal osc)
	38 seconds to load 14500 code words (most of the space) at 19200 / 8MHz (internal osc)
	Wired:
	11 seconds to load 14500 code words (most of the space) at 19200 / 8MHz (internal osc)
	so you see, there is no benefit to a higher baud rate. The XBee protocol is the bottleneck

	How to read the flash contents to file : 
	avrdude -c stk200 -p m168 -P lpt1 -Uflash:r:bl.hex:i
	This will dump the current flash contents of an AVR to a read-able hex file called "bl.hex". This
	was very helpful when testing whether flash writing was actually working.

	Oh, and if you happen to be using an XBee with a UFL antenna connector (and don't have a UFL antenna sitting around)
	you can convert it to a wire antenna simply by soldering in a short wire into the XBee. It may not be the best, 
	but it works.

*/

#define F_CPU 16000000
#define CPU_SPEED	16000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/boot.h>

//Define baud rate
#define BAUD   19200 //Works with internal osc
//#define BAUD   38400 //Works with internal osc
//#define BAUD   57600 //Does not work with internal osc
#define MYUBRR (((((CPU_SPEED * 10) / (16L * BAUD)) + 5) / 10) - 1)

//Here we calculate the wait period inside getch(). Too few cycles and the XBee may not be able to send the character in time. Too long and your sketch will take a long time to boot after powerup.
#define MAX_CHARACTER_WAIT	15 //10 works. 20 works. 5 throws all sorts of retries, but will work.
#define MAX_WAIT_IN_CYCLES ( ((MAX_CHARACTER_WAIT * 8) * CPU_SPEED) / BAUD )

//I have found that flow control is not really needed with this implementation of wireless bootloading.
//Adding flow control for wireless support
//#define sbi(port_name, pin_number)   (port_name |= 1<<pin_number)
//#define cbi(port_name, pin_number)   (port_name &= (uint8_t)~(1<<pin_number))
//#define CTS		2 //This is an input from the XBee. If low, XBee is busy - wait.
//#define RTS		3 //This is an output to the XBee. If we're busy, pull line low to tell XBee not to transmit characters into ATmega's UART

#define TRUE	0
#define FALSE	1

//Status LED
#define LED_DDR  DDRD
#define LED_PORT PORTD
#define LED_PIN  PIND
#define LED      PIND5

//Function prototypes
void putch(char);
char getch(void);
void flash_led(uint8_t);
void onboard_program_write(uint32_t page, uint8_t *buf);
void (*main_start)(void) = 0x0000;

//Variables
uint8_t incoming_page_data[256];
uint8_t page_length;
uint8_t retransmit_flag = FALSE;

union page_address_union {
	uint16_t word;
	uint8_t  byte[2];
} page_address;


union checksum_union {
	uint32_t word;
	uint8_t  byte[4];
} check_sum;



// http://en.wikipedia.org/wiki/Jenkins_hash_function
void jenkins_one_at_a_time_hash(uint32_t *hash, uint8_t *key, uint8_t key_len)
{
    for (uint8_t i = 0; i < key_len; i++) {
        *hash += key[i];
        *hash += (*hash << 10);
        *hash ^= (*hash >> 6);
    }
    *hash += (*hash << 3);
    *hash ^= (*hash >> 11);
    *hash += (*hash << 15);
}


int main(void)
{
	uint16_t i;
    check_sum.word = 0;

    //Setup USART baud rate
    UBRRH = MYUBRR >> 8;
    UBRRL = MYUBRR;
    UCSRB = (1<<RXEN)|(1<<TXEN);

	//set LED pin as output
	LED_DDR |= _BV(LED);

	//flash onboard LED to signal entering of bootloader
	flash_led(1);

	//Start bootloading process

	putch(5); //Tell the world we can be bootloaded

	//Check to see if the computer responded
	uint32_t count = 0;
	while(!(UCSRA & _BV(RXC)))
	{
		count++;
		if (count > MAX_WAIT_IN_CYCLES)
			main_start();
	}
	if(UDR != 6) main_start(); //If the computer did not respond correctly with a ACK, we jump to user's program

	while(1)
	{
		//Determine if the last received data was good or bad
        if (check_sum.word != 0) //If the check sum does not compute, tell computer to resend same line
RESTART:
            putch(7); //Ascii character BELL
        else            
            putch('T'); //Tell the computer that we are ready for the next line
        
        while(1) //Wait for the computer to initiate transfer
		{
			if (getch() == ':') break; //This is the "gimme the next chunk" command
			if (retransmit_flag == TRUE) goto RESTART;
		}

        page_length = getch(); //Get the length of this block
		if (retransmit_flag == TRUE) goto RESTART;

        if (page_length == 'S') //Check to see if we are done - this is the "all done" command
		{
			boot_rww_enable (); //Wait for any flash writes to complete?
			main_start(); 
		}
	
		//Get the memory address at which to store this block of data
		page_address.byte[0] = getch(); if (retransmit_flag == TRUE) goto RESTART;
		page_address.byte[1] = getch(); if (retransmit_flag == TRUE) goto RESTART;

        for (i = 0; i < 4; i++)
        {
            check_sum.byte[i] = getch(); //Pick up the check sum for error dectection
        	if (retransmit_flag == TRUE) goto RESTART;
        }
		
		for(i = 0 ; i < page_length ; i++) //Read the program data
		{
            incoming_page_data[i] = getch();
			if (retransmit_flag == TRUE) goto RESTART;
		}
 
        //Calculate the checksum
        uint32_t hash = 0;
        jenkins_one_at_a_time_hash(&hash, incoming_page_data, page_length);
        jenkins_one_at_a_time_hash(&hash, &page_length, 1);
        jenkins_one_at_a_time_hash(&hash, page_address.byte, 1);
		jenkins_one_at_a_time_hash(&hash, page_address.byte+1, 1);

        check_sum.word -= hash;
        
		/*
        for(i = 0 ; i < page_length ; i++)
            check_sum = check_sum + incoming_page_data[i];
        
        check_sum = check_sum + page_length;
        check_sum = check_sum + page_address.byte[0];
        check_sum = check_sum + page_address.byte[1];
        */
		
        if(check_sum.word == 0) //If we have a good transmission, put it in ink
            onboard_program_write((uint32_t)page_address.word, incoming_page_data);
	}

}


void onboard_program_write(uint32_t page, uint8_t *buf)
{
	uint16_t i;
	//uint8_t sreg;

	// Disable interrupts.

	//sreg = SREG;
	//cli();

	//eeprom_busy_wait ();

	boot_page_erase (page);
	boot_spm_busy_wait ();      // Wait until the memory is erased.

	for (i=0; i<SPM_PAGESIZE; i+=2)
	{
		// Set up little-endian word.

		uint16_t w = *buf++;
		w += (*buf++) << 8;
	
		boot_page_fill (page + i, w);
	}

	boot_page_write (page);     // Store buffer in flash page.
	boot_spm_busy_wait();       // Wait until the memory is written.

	// Reenable RWW-section again. We need this if we want to jump back
	// to the application after bootloading.

	//boot_rww_enable ();

	// Re-enable interrupts (if they were ever enabled).

	//SREG = sreg;
}

void putch(char ch)
{
	//Adding flow control - xbee testing
	//while( (PIND & (1<<CTS)) != 0); //Don't send anything to the XBee, it is thinking

	while (!(UCSRA & _BV(UDRE)));
	UDR = ch;
}

char getch(void)
{
	retransmit_flag = FALSE;
	
	//Adding flow control - xbee testing
	//cbi(PORTD, RTS); //Tell XBee it is now okay to send us serial characters

	uint32_t count = 0;
	while(!(UCSRA & _BV(RXC)))
	{
		count++;
		if (count > MAX_WAIT_IN_CYCLES) //
		{
			retransmit_flag = TRUE;
			break;
		}
	}

	//Adding flow control - xbee testing
	//sbi(PORTD, RTS); //Tell XBee to hold serial characters, we are busy doing other things

	return UDR;
}

void flash_led(uint8_t count)
{
	uint8_t i;
	
	for (i = 0; i < count; ++i) {
		LED_PORT |= _BV(LED);
		_delay_ms(100);
		LED_PORT &= ~_BV(LED);
		_delay_ms(100);
	}
}

