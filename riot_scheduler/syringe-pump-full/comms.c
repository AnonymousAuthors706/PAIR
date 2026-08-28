#include <stdint.h>
#include "hardware.h"
#include "comms.h"

/************ UART COMS ************/
char sim_input[16] = "+\n00000000000000";

void recvBuffer(uint16_t * uart_status, uint8_t * rx_data, uint16_t size){
    //P1OUT = rx_data[size-1];
    //P1OUT ^= 0x40;
    unsigned int i=0, j;
    while(i < TOTAL_RECV){
		
		#if IS_SIM == NOT_SIM
		rx_data[i] = UART_RXD;
		// implementation only
		for(j=0; j<DELAY; j++)
		{} // wait for buffer to clear before reading next char
		#else
		// simulation mode, feed in input from a tmp buffer
		for(j=0; j<DELAY; j++);
		//P1OUT = i;
		
		if(i < size)
			rx_data[i] = sim_input[i];
		else{
			*uart_status = 1;
			thread_yield();
		}	
		
		#endif
		i++;
    }
    //P1OUT ^= 0x40;
	thread_zombify();
}

void sendBuffer(uint16_t * uart_status, uint8_t * tx_data, uint16_t size){

    P1OUT = tx_data[size-1];

    unsigned int i, j;
    for(i=0; i<size; i++){
        UART_TXD = tx_data[i];
      
        #if IS_SIM == NOT_SIM
        // only implementation
        for(j=0; j<DELAY; j++)
        {} // wait for buffer to clear before sending next char
        #endif
    }
	*uart_status = 1;
	thread_zombify();
}