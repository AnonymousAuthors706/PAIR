/**
 * Muhammad Wasif Kamran
 *
 * RIOT Example #1: Thread creation without yield
 *
 * Expected behaviour:
 *      P3OUT : 0x50, 0x02, 0x03, ... , 0x0c, 0xde, 0xdf, ....
 *      P1OUT : 0xee,                 0xdd
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

char blink_stack[THREAD_STACKSIZE_MEDIUM];
char second_thread_stack[THREAD_STACKSIZE_MEDIUM];
char third_thread_stack[THREAD_STACKSIZE_MEDIUM];

// #define THREAD_1_STACK 0x2800
// #define THREAD_2_STACK THREAD_1_STACK+THREAD_STACKSIZE_MEDIUM
// #define THREAD_3_STACK THREAD_2_STACK+THREAD_STACKSIZE_MEDIUM

// char * blink_stack = ((char*)(THREAD_1_STACK));
// char * second_thread_stack = ((char*)(THREAD_2_STACK));
// char * third_thread_stack = ((char*)(THREAD_3_STACK));

void *second_thread(void *arg);
void *third_thread(void *arg);
void *blink_thread(void *arg);

/**
 * This thread will execute only once before falling asleep (never wakes up again)
 */
#define TOTAL_MID_THREADS 3 // total threads is +2
int total_blinks;
void *blink_thread(void *arg)
{
        (void)arg;

        thread_create(third_thread_stack, sizeof(third_thread_stack), 0,
                      THREAD_CREATE_WOUT_YIELD, third_thread, NULL, "ping");

        for(int i=0; i<=TOTAL_MID_THREADS; i++)
        {
                thread_create(second_thread_stack, sizeof(second_thread_stack), 1,
                      THREAD_CREATE_WOUT_YIELD, second_thread, NULL, "pong");
        }

        blink_pid = thread_getpid();

        int x = 0;

        while (x < 256) {
                // P1OUT = x % 2 == 0 ? 0xff : 0xee;
                // ++x;
                //if ((x & 0x000f) == 0x000f){
                //    thread_yield();
                //}
                x++;
        }
        // thread_sleep();
        thread_zombify();
        return NULL;
}

/**
 * second_thread and third_thread pass a message back and forth while second thread increases the
 * value in the message when the message value becomes 12, the second thread zombifies itself
 */
void *second_thread(void *arg)
{
        (void)arg;

        // second_pid = thread_getpid();

        // msg_t m;
        // m.content.value = 0;
        int x = 0;
        while (x < 256) {
                // msg_receive(&m);

                // m.content.value++;
                // msg_reply(&m, &m);
                if ((x & 0x000f) == 0x000f){
                    thread_yield();
                }
                x++;
        }

        P1OUT = 0xdd; /* second_thread becomes zombified (exited loop) */

        thread_zombify();
        // blink_thread(NULL);


        return NULL;
}

void *third_thread(void *arg)
{
        (void)arg;

        // msg_t m;
        // m.content.value = 1;

        int x = 0;

        while (x < 256) {
                // /* after second_thread is zombified, P3OUT should continuously keep increasing */
                // if (thread_get_status(thread_get_unchecked(second_pid)) == STATUS_ZOMBIE) {
                //         P3OUT = x;
                //         ++x;
                // }

                // else {
                //         msg_send_receive(&m, &m, second_pid);
                //         P3OUT = m.content.value; /* display current message value in P3OUT => P3OUT
                //                                     should continuously increase to 12 before
                //                                     becoming 0xde and increasing forever */
                // }
                // m.content.value++;
                if ((x & 0x000f) == 0x000f){
                    thread_yield();
                }
                x++;
        }
        thread_zombify();
        return NULL;
}

int main(void)
{
        uint32_t* wdt = (uint32_t*)(WDTCTL_);
        *wdt = WDTPW | WDTHOLD;

        P3OUT = 0x50;

        thread_create(blink_stack, sizeof(blink_stack), 2,
                      THREAD_CREATE_WOUT_YIELD, blink_thread, NULL, "blink");


        /* due to THREAD_CREATE_WOUT_YIELD, execution does not start in any of the threads until the
         * following call to thread_yield() which gives execution to the highest priority thread
         * (blink_thread) */

        thread_yield();

        // blink_thread(NULL);
}
