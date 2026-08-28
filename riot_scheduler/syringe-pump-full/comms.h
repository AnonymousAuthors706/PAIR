#ifndef COMMS_H
#define COMMS_H

#define UART_TIMEOUT   0x1000

#define NOT_SIM 0
#define SIM 1
#define IS_SIM SIM

#define DELAY 500
#define TOTAL_RECV 4

void recvBuffer(uint16_t * uart_status, uint8_t * rx_data, uint16_t size);
void sendBuffer(uint16_t * uart_status, uint8_t * tx_data, uint16_t size);

#endif // COMMS_H
