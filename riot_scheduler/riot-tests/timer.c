/**
 * Muhammad Wasif Kamran
 * 
 * RIOT Example #3: Timer based thread
 * 
 * Expected behaviour:
 *      P3OUT : 0x50, 0x02, 0x03, ..., 0x0c, 0xde, 0xdf, ...
 *      P1OUT : 0xee, 0xff, 0xee, 0xff, ...
 * 
 * Approx. sim time: 15ms
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

kernel_pid_t second_pid, blink_pid;

char second_thread_stack[THREAD_STACKSIZE_MEDIUM];
char third_thread_stack[THREAD_STACKSIZE_MEDIUM];
char blink_stack[THREAD_STACKSIZE_MEDIUM];

void *second_thread(void *arg);
void *third_thread(void *arg);
void *blink_thread(void *arg);

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

__attribute__((interrupt TIMERA0_VECTOR)) void TIMER_ISR(void)
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

        int x = 0xde;

        while (1) {
                /* after second_thread is zombified, P3OUT should continuously keep increasing */
                if (thread_get_status(thread_get_unchecked(second_pid)) == STATUS_ZOMBIE) {
                        P3OUT = x;
                        ++x;
                }

                else {
                        msg_send_receive(&m, &m, second_pid);
                        P3OUT = m.content.value; /* display current message value in P3OUT => P3OUT
                                                    should continuously increase to 12 before
                                                    becoming 0xde and increasing forever */
                }
        }

        return NULL;
}

int main(void)
{
        /* setup the hardware timer and enable interrupts */
        setup();
        __enable_interrupt();

        P3OUT = 0x50;

        thread_create(second_thread_stack, sizeof(second_thread_stack), THREAD_PRIORITY_MAIN - 3, 0,
                      second_thread, NULL, "pong");
}
