/**
 * Muhammad Wasif Kamran
 * 
 * RIOT Example #4: Software update functionality 
 * 
 * Expected behaviour:
 *      P3OUT : 0x50, 0x02, 0x03, ..., 0x0c, XX, 0xab, ..., 0xfa
 *      P1OUT : 0xee, 0xff, 0xee, 0xff, ...
 * 
 * Approx. sim time: 20ms
 * 
 */


#include <inttypes.h>
#include <stdio.h>

#include "hardware.h"
#include "irq.h"
#include "msg.h"
#include "thread.h"

// Watchdog timer
#define WDTCTL_ 0x0120 /* Watchdog Timer Control */
#define WDTHOLD (0x0080)
#define WDTPW (0x5A00)

/* software update addresses */
#define VIOLATE_FLAG_ADDR 0x400
#define VIOLATE_ID_ADDR 0x402

/* bounds for "task" to be updated */
int bounds[2] = {0x2048, 0x204e};

kernel_pid_t second_pid, blink_pid;

char second_thread_stack[THREAD_STACKSIZE_MEDIUM];
char third_thread_stack[THREAD_STACKSIZE_MEDIUM];
char blink_stack[THREAD_STACKSIZE_MEDIUM];

void *second_thread(void *arg);
void *third_thread(void *arg);
void *blink_thread(void *arg);

uint8_t hmac_prv[32];
uint8_t key[32] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
                   0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                   0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

uint8_t newbytes[] = {0xab, 0xbc, 0xcd, 0xde, 0xef, 0xfa};
uint8_t vrf_hmac[] = {0x0a, 0x89, 0x4d, 0xf7, 0x7a, 0xc6, 0x46, 0x15, 0x60, 0xe6, 0x47,
                      0x56, 0xe0, 0x6d, 0xcf, 0x7b, 0xd8, 0x16, 0xc0, 0x02, 0x8e, 0xaf,
                      0xf3, 0xc8, 0xc9, 0x3a, 0x56, 0xd8, 0xd3, 0x09, 0x16, 0xa5};

uint8_t update_buffer_uart[128];
uint8_t hmac_vrf_buffer[32];

/*** USE MY_HMAC FOR SIMULATION PURPOSES ***/
/* extern void hmac(uint8_t *mac, uint8_t *key, uint32_t keylen, uint8_t *data, uint32_t datalen);
 */

void my_hmac(uint8_t *mac, uint8_t *key, uint32_t keylen, uint8_t *data, uint32_t datalen)
{
        unsigned int i;
        unsigned int j;
        for (i = 0; i < keylen; i++) {
                mac[i] = key[i] | data[i];
        }
}

void report_generation(uint8_t *update)
{
        my_hmac(hmac_prv, key, 32, (uint8_t *)(update), 32);
}

void sw_update(uint8_t *update, uint8_t *hmac_vrf)
{
        if (!(*(uint8_t *)VIOLATE_FLAG_ADDR))
                return;

        // generate hmac_prv

        report_generation(update);

        // compare prv and vrf hmac (proceed only if same hmac)

        // for (int i = 0; i < 32; ++ i) {
        //     if (hmac_prv[i] != hmac_vrf[i]) return;
        // }

        // overwrite existing code

        int task_id = *(uint8_t *)VIOLATE_ID_ADDR;

        uint8_t *iter_ptr = (uint8_t *)bounds[(task_id - 1) * 2];

        for (int i = 0; i < bounds[(task_id - 1) * 2 + 1] - bounds[(task_id - 1) * 2]; ++i) {
                *iter_ptr = update[i];
                ++iter_ptr;
        }
}

/* receive software update over UART */

int UART_RX_COUNTER = 0;

__attribute__((interrupt UART_RX_VECTOR)) void RX_ISR(void)
{
        int32_t TASK_SIZE = bounds[((*(uint8_t *)VIOLATE_ID_ADDR) - 1) * 2 + 1] -
                            bounds[((*(uint8_t *)VIOLATE_ID_ADDR) - 1) * 2];

        uint8_t cbyte = UART_RXD;

        if (UART_RX_COUNTER < TASK_SIZE) {
                update_buffer_uart[UART_RX_COUNTER] = cbyte;
                ++UART_RX_COUNTER;
        } else {
                if (UART_RX_COUNTER - TASK_SIZE <= 32) {
                        hmac_vrf_buffer[UART_RX_COUNTER - 32] = cbyte;
                        ++UART_RX_COUNTER;
                } else {
                        sw_update(update_buffer_uart, hmac_vrf_buffer);
                        UART_RX_COUNTER = 0;
                }
        }
}

/**
 * Sets up the hardware timer to fire the ISR every 2000 units (reasonable for Vivado viewing)
 *      Also disables the watchdog
 */
void setup(void)
{
        // Disables WDT
        WDTCTL = WDTPW | WDTHOLD; // Disable watchdog timer

        // Timer interrupt configuration
        CCTL0 = CCIE; // CCR0 interrupt enabled
        CCR0 = 2000;  // for vivado simulation
        //  CCR0  = 10000; // FPGA visible (slow) speed
        TACTL = TASSEL_2 + MC_1 + ID_3; // SMCLK, contmode

        P3DIR = 0xFF;
        P3OUT = 0x00;
}

__attribute__((interrupt TIMERA0_VECTOR)) void TCB(void)
{
        if (thread_get_status(thread_get_unchecked(blink_pid)) == STATUS_SLEEPING)
                thread_wakeup(blink_pid);

        CCTL0 &= ~CCIFG;
}

/**
 * This thread should wake up everytime the hardware timer ISR fires, meaning P1OUT should switch
 * between 0xff and 0xee every few ns
 */
void *blink_thread(void *arg)
{
        (void)arg;

        blink_pid = thread_getpid();

        int x = 1;

        while (1) {
                P1OUT = x % 2 == 0 ? 0xff : 0xee;
                ++x;

                thread_sleep();
        }

        return NULL;
}

/**
 * second_thread and third_thread pass a message back and forth while the second_thread is
 * responsible for increasing the value of the message when message value becomes 12, second_thread
 * zombifies itself
 */
void *second_thread(void *arg)
{
        (void)arg;

        second_pid = thread_getpid();

        thread_create(third_thread_stack, sizeof(third_thread_stack), THREAD_PRIORITY_MAIN - 2, 0,
                      third_thread, NULL, "ping");

        thread_create(blink_stack, sizeof(blink_stack), THREAD_PRIORITY_MAIN - 4, 0, blink_thread,
                      NULL, "blink");

        msg_t m;
        m.content.value = 0;

        while (m.content.value < 12) {
                msg_receive(&m);

                m.content.value++;
                msg_reply(&m, &m);
        }

        thread_zombify();

        return NULL;
}

void *third_thread(void *arg)
{
        (void)arg;

        msg_t m;
        m.content.value = 1;

        while (1) {
                if (thread_get_status(thread_get_unchecked(second_pid)) == STATUS_ZOMBIE) {
                        /**
                         * Once the second thread becomes a zombie, this will perform a software
                         * update using the bounds and bytes defined at the beginning P3OUT will
                         * display the values at 0x2048 (+ 6) before and after the software update
                         */

                        *(uint8_t *)VIOLATE_FLAG_ADDR = 1;
                        *(uint8_t *)VIOLATE_ID_ADDR = 1;

                        uint8_t *iterp = (uint8_t *)0x2048;
                        for (int i = 0; i < 6; ++i) {
                                P3OUT = *iterp;
                                ++iterp;
                        }

                        sw_update(newbytes, vrf_hmac);

                        uint8_t *iter = (uint8_t *)0x2048;
                        for (int i = 0; i < 6; ++i) {
                                P3OUT = *iter;
                                ++iter;
                        }

                        thread_zombify();
                }

                else {
                        msg_send_receive(&m, &m, second_pid);
                        P3OUT = m.content.value;
                }
        }
}

int main(void)
{
        setup();
        __enable_interrupt();

        P3OUT = 0x50;

        thread_create(second_thread_stack, sizeof(second_thread_stack), THREAD_PRIORITY_MAIN - 3, 0,
                      second_thread, NULL, "pong");
}
