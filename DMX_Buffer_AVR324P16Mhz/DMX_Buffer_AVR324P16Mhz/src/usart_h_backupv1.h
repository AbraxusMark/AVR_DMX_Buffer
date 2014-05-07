/*
 * usart.h
 *
 *  Created on: 27-jan-2009
 *      Author: Jan
 */

#ifndef USART_H_
#define USART_H_

void USART_Init(unsigned int baudrate)
{
	//UART RECEIVER
	
	/* Set baud rate */
	UBRR0H = (unsigned char)(baudrate>>8);
	UBRR0L = (unsigned char)baudrate;
	UBRR1H = (unsigned char)(baudrate>>8);
	UBRR1L = (unsigned char)baudrate;
	/* Enable receiver and interrupt*/
	UCSR0B = (1<<RXEN0)|(1<<RXCIE0); //Turn on on UART 0 RX & int
	UCSR0C = 0x00;
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01)|(1<<USBS0); //8 bit async - USBS0 actually superfluous
	
	// Init TX - 8N2
	UCSR1B = (1<<TXEN1); // |(1<<TXCIE0); for now don't enable TX complete interupt
	UCSR1C = 0x00;
	UCSR1C = (1<<UCSZ10)|(1<<UCSZ11)|(1<<USBS1);		
	
}

void USART_InitTX(unsigned int baudrate)
{	
	//UART Transmitter
	while(!(UCSR1A & (1<<UDRE1)));
	
	UBRR1H = (unsigned char)(baudrate>>8);
	UBRR1L = (unsigned char)baudrate;
	
// 	if (baudrate == 3) //full scale
// 	{
// 		UCSR1C &= ~(1<<UPM11); //no parity
// 	}
// 	else{
// 		UCSR1C |= (1<<UPM11); //even parity on break?
// 	}	
}

void USART_Transmit( unsigned char data )
{
/* Wait for empty transmit buffer */
while ( !( UCSR1A & (1<<UDRE1)) )
;
/* Put data into buffer, sends the data */
UDR1 = data;
}


#endif /* USART_H_ */
