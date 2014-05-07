/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
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
#define DMX_BREAK_Scale 9
#define LED_PORT	PORTB			//Port where the LED's are located .. Duh! :P
#define LED_DDR		DDRB			//Data Direction Register of the LED's..
#define DMX_ADDRESS_DDR		DDRA	//DDR where the "usual" Dipswitch is on for the channel setting
#define DMX_ADDRESS_PORT	PORTA	//Port with the "usual" Dipswitch for the channel setting
#define DMX_ADDRESS_PIN		PINA	//Pins where the "usual" Dipswitch for the channel setting


//Functions
void Init_Ports();	//Initialize IO Pins
void TransmitDMXFrame();

//Variables
volatile unsigned char vuchrDMX_Break,UART_Status,DMX_Data;			//"Boolean" which holds the break detection
volatile unsigned int  vintDMX_ChCounter;							//For counting the data packets
volatile unsigned char vDMXPacket[515];								// 0 index is start ?
volatile unsigned int vdiv;
unsigned int i = 0;
volatile unsigned char vucButtonStates = 0xFF;
volatile uint8_t vintStartAddress = 0;

//Main Loop
int main(){
	Init_Ports();								//Call the Init_Ports() function.
	USART_Init(DMX_FULL_Scale);					//USART init (250Kbps, 8 databits, 2 stopbits)
																			
	wdt_disable();								//Disable the Watchdog so the controller won't reset on infinite loops.							
	
	for(i=0;i<515;i++){
		vDMXPacket[i] = 0x00;
	}
	
	sei();										//Enable all interrupts
	////////////////////////////////			Main Loop
	for(;;){
		//_delay_us(40);
		TransmitDMXFrame();
 		vucButtonStates = PINA;
 		PORTB = vucButtonStates;
		vintStartAddress = (int)~vucButtonStates;
		
	}
	

	
	return 0;
}


//Functions
void Init_Ports(){					
	
	LED_DDR = 0xFF;					//DDR = 255 => Output
	PORTB = 0xFF;					// zero out LED port
	DMX_ADDRESS_DDR=0; 				//Input
	DMX_ADDRESS_PORT=0xFF;			//pull ups
	PCICR |= (1 << PCIE0);			// set PCIE0 to enable PCMSK0 scan
	PCMSK0 = 0xFF;		// set PORT to trigger an interrupt on state change 
}


void TransmitDMXFrame(){
	
	// BREAK
	USART_InitTX(DMX_BREAK_Scale);
	UDR1 = 0x00;
	while(!(UCSR1A & (1 << UDRE1))) ;
	// PACKET
	USART_InitTX(DMX_FULL_Scale);
	
	for(i=1;i<514;i++){						//idx 0 is start packet?
		while(!(UCSR1A & (1 << UDRE1))) ;
		UDR1 = vDMXPacket[i];
	}	
}

//Interrupts
ISR(USART0_RX_vect){						//UART 0 ISR

	int temp = 0;

	UART_Status = UCSR0A;						//Read the status from the UCSRA register
	DMX_Data = UDR0;							//Read data from UDR data register

	if (UART_Status & (1<<FE0)){				//If we detect a frame error=> RX_line low for longer then 8 bits (Break condition)
		vuchrDMX_Break=1;						//We have a break..
		vintDMX_ChCounter=0;					//Reset the Channel Counter.
		
	}
	else if(vuchrDMX_Break==1){					//If a break was detected..
		temp = vintDMX_ChCounter-vintStartAddress;
		if(temp>=0) 
		{
			vDMXPacket[temp] = DMX_Data;
		}
		
		vintDMX_ChCounter++;					//Increase the Channel Counter
	}
}


// ISR(PCINT0_vect){
// 	ATOMIC_BLOCK(ATOMIC_FORCEON){
// 	_delay_ms(100);
// 	vucButtonStates ^= PINA;
// 	PORTB = ~vucButtonStates;
// 	vintStartAddress = vucButtonStates;
// 	}
// }
