/**
 * \main.c
 *
 * Attempt at DMX buffer
 *
 * 2nd June 14 - Modded for PCB
 */


#include <asf.h>
#include <util\delay.h>
#include <util\atomic.h>
#include <avr\io.h>
#include "inttypes.h"
#include "stdio.h"
#include "avr/interrupt.h"
#include "usart.h"
#include "avr/wdt.h"

#define F_CPU 16000000L			//16Mhz crystal 
#define DMX_FULL_Scale 3		//=(F_CPU)/(16*Baudrate).. For 16Mhz crystal and 250000bps => 3

#define DMX_ADDRESS_DDR		DDRA	//DDR - Dipswitch is on for the channel setting
#define DMX_ADDRESS_PORT	PORTA	//Port - Dipswitch for the channel setting
#define DMX_ADDRESS_PIN		PINA	//Pins - Dipswitch for the channel setting
//#define DMXFRAME	512 
#define DMXTXPIN	PIND3		


//DMX Params

#define BREAKTIME 170
#define MABTIME 8
//#define STARTTIME 40
#define MBSTIME 30
#define MBBTIME 800

//Functions
void Init_Ports(void);								//Initialise IO Pins
int main(void);
int positiveonly(int);

//Variables

volatile unsigned int  vintDMX_ChCounter;			//For counting the data packets
volatile unsigned char vDMXPacket[514];			// 0 index is start frame, 2 x 513 for flip flop buffer
volatile unsigned char vucharDIPStates[2];
volatile uint16_t vintStartAddress = 0;

//Main Loop
int main(){
	
	unsigned int i = 0;
	
	Init_Ports();								//Call the Init_Ports() function.
	USART_Init(DMX_FULL_Scale);					//USART init (250Kbps, 8 databits, 2 stopbits)
																			
	wdt_disable();								//Disable the Watchdog so the controller won't reset on infinite loops.							
	
	for(i=0;i<513;i++){
		vDMXPacket[i] = 0x00;
	}
	sei();										//Enable all interrupts
											
	////////////////////////////////			Main Loop
	for(;;){
		
		vucharDIPStates[0] = ~(DMX_ADDRESS_PIN);
		vucharDIPStates[1] = !(PINB & (1 << PINB0));								//select out the 1st bit as for 256
		ATOMIC_BLOCK(ATOMIC_FORCEON){												//16 bit access
			vintStartAddress = vucharDIPStates[0] | (vucharDIPStates[1] << 8);
		}
		
		_delay_us(BREAKTIME);														//BREAK
		
																					// MARK AFTER BREAK
		PORTD |= (1<<DMXTXPIN);
						//PORTD |= (1<<PIND6);	//************ DEBUG cycle gen
		_delay_us(MABTIME);
						//PORTD &= ~(1<<DMXTXPIN);
						//_delay_us(STARTTIME);
		PORTD |= (1<<DMXTXPIN);
		
																					// PACKET
		UCSR1A = (1<<TXC1);
		UCSR1B |= (1<<TXEN1);

			for(i=0;i<513;i++){		
				while(!(UCSR1A & (1 << UDRE1))) ;
				UDR1 = vDMXPacket[i];	
											//_delay_us(MBSTIME);		//SLOT delay
			}			
		

		while (!(UCSR1A & (1<<TXC1)));
		UCSR1B &= ~(1<<TXEN1);														//uart tx off for rest of timings
	
																					//MARK BEFORE BREAK	
		PORTD |= (1<<DMXTXPIN);
		_delay_us(MBBTIME);
		PORTD &= ~(1<<DMXTXPIN);
										//PORTD &= ~(1<<PIND6);	//************ DEBUG cycle gen
		PORTB ^= (1<<PINB2);														//LED status blink
			
	}
	return 0;
}

//Functions
void Init_Ports(void){					
	
	
	DMX_ADDRESS_DDR=0; 				//0-7 DIP Input
	DMX_ADDRESS_PORT=0xFF;			//pull ups
	DDRD |= (1<<DMXTXPIN);			//USART PIN TX set output
	PORTD &= ~(1<<DMXTXPIN);		// ^ pin is low for 0 / idle
	//DDRD |= (1<<DDRD6);				//TEST PIN FOR SCOPE
	
	//set B0 & B1 for dip input, B2 & B3 for LED out
	
	DDRB = 0b11111100; 	
	PORTB = 0xFF;				// PIN B0 & B1 Pullup, and heck why not all on at start.
						
}

//Interrupts
ISR(USART0_RX_vect){							//UART 0 ISR

	
	unsigned char UART_Status = UCSR0A;						//Read the status from the UCSRA register
	unsigned char DMX_Data = UDR0;							//Read data from UDR data register

	if (UART_Status & (1<<FE0)){				//If we detect a frame error=> RX_line low for longer then 8 bits (Break condition)
		
		vintDMX_ChCounter=0;					//Reset the Channel Counter.
		PORTB ^= (1<<PINB3);
	}
	else {
	int temp =  positiveonly(vintDMX_ChCounter-vintStartAddress);
		if(temp>0){
			vDMXPacket[temp] = DMX_Data;
		}
		else if (temp == 0 ){					//recreate start bit
			vDMXPacket[temp] = 0x00;
		}
		
		vintDMX_ChCounter++;					//Increase the Channel Counter
	}


}

int positiveonly(int number)
{
	
	if(number >= 0) return number;
	else return 0;
	
}