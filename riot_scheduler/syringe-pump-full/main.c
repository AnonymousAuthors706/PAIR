#include "thread.h"
#include <stdio.h>
#include "hardware.h"
//#include "lcd.h"
#include "temp.h"
#include "syringe.h"
#include "comms.h"

/* All four threads share one priority -> true round-robin via thread_yield() */
#define PRIO_ALL       (THREAD_PRIORITY_MAIN)
#define STACKSIZE      (THREAD_STACKSIZE_DEFAULT)
// Watchdog timer
#define WDTCTL_              0x0120    /* Watchdog Timer Control */
#define WDTHOLD             (0x0080)
#define WDTPW               (0x5A00)

char comm_stack[STACKSIZE];
char syringe_stack[STACKSIZE];
char temp_stack[STACKSIZE];
char lcd_stack[STACKSIZE];

int latest_temp;
int syringe_status;

char input[16];// = "+\n"
int uart_status = 0; // 0 = not done, 1 = done
void *comm_recv_thread(void *arg)
{
    (void)arg;

    while (1) {
        recvBuffer(&uart_status, input, 2);
        thread_yield();   /* equal priority -> this actually rotates to the next thread now */
    }
    return NULL;
}

int syringe_status = 0; // 0 = not running, 1 running
void *syringe_thread(void *arg)
{
    (void)arg;

    while (1) {
		if (uart_status == 1)
			syringe_control(input, &syringe_status);
		else
			thread_yield();
    }
    return NULL;
}

void *temp_thread(void *arg)
{
    (void)arg;

    while (1) {
        get_temperature();
        thread_yield();
    }
    return NULL;
}

void *lcd_thread(void *arg)
{
    (void)arg;

    while (1) {
        run_lcd(latest_temp, syringe_status);
    }
    return NULL;
}

int main(void)
{
	uint32_t* wdt = (uint32_t*)(WDTCTL_);
    *wdt = WDTPW | WDTHOLD;
	
    thread_create(comm_stack, sizeof(comm_stack), PRIO_ALL,
                  THREAD_CREATE_STACKTEST, comm_recv_thread, NULL, "comm_recv");

    thread_create(syringe_stack, sizeof(syringe_stack), PRIO_ALL,
                  THREAD_CREATE_STACKTEST, syringe_thread, NULL, "syringe");

    thread_create(temp_stack, sizeof(temp_stack), PRIO_ALL,
                  THREAD_CREATE_STACKTEST, temp_thread, NULL, "temp");

    thread_create(lcd_stack, sizeof(lcd_stack), PRIO_ALL,
                  THREAD_CREATE_STACKTEST, lcd_thread, NULL, "lcd");

	thread_yield();
    return 0;
}