/**
 * \main.c
 *
 * Attempt at DMX buffer
 *
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

#define F_CPU 16000000L			//16Mhz crystal used for baud rate 0.0% error
#define DMX_FULL_Scale 3		//=(F_CPU)/(16*Baudrate).. For 16Mhz crystal and 250000bps => 3
// #define DMX_BREAK_Scale 9
#define LED_PORT	PORTB			//Port where the LED's are located .. Duh! :P
#define LED_DDR		DDRB			//Data Direction Register of the LED's..
#define DMX_ADDRESS_DDR		DDRA	//DDR where the "usual" Dipswitch is on for the channel setting
#define DMX_ADDRESS_PORT	PORTA	//Port with the "usual" Dipswitch for the channel setting
#define DMX_ADDRESS_PIN		PINA	//Pins where the "usual" Dipswitch for the channel setting
#define DMXFRAME 512 
#define NOP()   __asm__ __volatile__ ("nop" ::)
#define DMXTXPIN	PIND3		


//DMX Params

#define BREAKTIME 170
#define MABTIME 9
#define STARTTIME 40
#define MBSTIME 30
#define MBBTIME 150

//Functions
void Init_Ports();	//Initialize IO Pins
void TransmitDMXFrame();

//Variables

volatile unsigned int  vintDMX_ChCounter;							//For counting the data packets
volatile unsigned char vDMXPacket[514];								// 0 index is start ?
volatile unsigned int vdiv;

volatile unsigned char vucButtonStates = 0xFF;
volatile uint8_t vintStartAddress = 0;
unsigned char vuchrDMX_Break;

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
		
		vucButtonStates = PINA;
		//PORTB = vucButtonStates;
		vintStartAddress = (int)~vucButtonStates;
				
		_delay_us(BREAKTIME);						//BREAK
		
		// MARK AFTER BREAK
		PORTD |= (1<<DMXTXPIN);
										PORTD |= (1<<PIND6);	//************ DEBUG cycle gen
		_delay_us(MABTIME);
		//PORTD &= ~(1<<DMXTXPIN);
		//_delay_us(STARTTIME);
		PORTD |= (1<<DMXTXPIN);
		// PACKET
		UCSR1A = (1<<TXC1);

		for(i=0;i<513;i++){		
			UCSR1B |= (1<<TXEN1);
			while(!(UCSR1A & (1 << UDRE1))) ;
			UDR1 = vDMXPacket[i];
								
			while (!(UCSR1A & (1<<TXC1)));
			UCSR1B &= ~(1<<TXEN1);	
		
			_delay_us(MBSTIME);		//SLOT delay
		}	
					//uart tx off for rest of timings
	
		//MARK BEFORE BREAK	
		PORTD |= (1<<DMXTXPIN);
		_delay_us(MBBTIME);
		PORTD &= ~(1<<DMXTXPIN);
										PORTD &= ~(1<<PIND6);	//************ DEBUG cycle gen
		PORTB ^= (1<<PINB6);						//LED status
			

	}
	
	return 0;
}

//Functions
void Init_Ports(){					
	
	LED_DDR = 0xFF;					//DDR = 255 => Output
	PORTB = 0xFF;					// zero out LED port
	DMX_ADDRESS_DDR=0; 				//Input
	DMX_ADDRESS_PORT=0xFF;			//pull ups
// 	PCICR |= (1 << PCIE0);			// set PCIE0 to enable PCMSK0 scan
// 	PCMSK0 = 0xFF;					// set PORT to trigger an interrupt on state change 
	DDRD |= (1<<DMXTXPIN);			//USART PIN TX set output
	PORTD &= ~(1<<DMXTXPIN);		// ^ pin is low for 0 / idle
	DDRD |= (1<<DDRD6);				//TEST PIN FOR SCOPE
}

//Interrupts
ISR(USART0_RX_vect){							//UART 0 ISR

	int temp = vintDMX_ChCounter-vintStartAddress;
	unsigned char UART_Status = UCSR0A;						//Read the status from the UCSRA register
	unsigned char DMX_Data = UDR0;							//Read data from UDR data register

	if (UART_Status & (1<<FE0)){				//If we detect a frame error=> RX_line low for longer then 8 bits (Break condition)
		vuchrDMX_Break=1;						//We have a break.
		vintDMX_ChCounter=0;					//Reset the Channel Counter.
							LED_PORT ^= (1<<PINB7);
	}
	else if(vuchrDMX_Break==1){					//If a break was detected..
		if(temp>0) 
		{
			vDMXPacket[temp] = DMX_Data;
		}
		else {
			vDMXPacket[vintDMX_ChCounter] = 0x00;
			
		}

		
		vintDMX_ChCounter++;					//Increase the Channel Counter
	}
}

