/*
 * Copyright (C) 2021 TUBA Freiberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
/**
 * @ingroup     sys
 * @{
 *
 * @file
 * @brief       Round Robin Scheduler implementation
 *
 * @author      Karl Fessel <karl.fessel@ovgu.de>
 *
 * @}
 */

#include "sched.h"
#include "thread.h"
// #include "sched_round_robin.h"
#if !defined(SCHED_RR_TIMEOUT) || defined(DOXYGEN)
/**
 * @brief   Time between round robin calls in Units of SCHED_RR_TIMERBASE
 *
 * @details Defaults to 10ms
 */
#if MODULE_ZTIMER_MSEC
#define SCHED_RR_TIMEOUT 10
#else
#define SCHED_RR_TIMEOUT 10000
#endif
#endif

#if !defined(SCHED_RR_TIMERBASE) || defined(DOXYGEN)
/**
 * @brief   ztimer to use for the round robin scheduler
 *
 * @details Defaults to ZTIMER_MSEC if available else it uses ZTIMER_USEC
 */
#if MODULE_ZTIMER_MSEC
#define SCHED_RR_TIMERBASE ZTIMER_MSEC
#else
#define SCHED_RR_TIMERBASE ZTIMER_USEC
#endif
#endif

#if !defined(SCHED_RR_MASK) || defined(DOXYGEN)
/**
 * @brief   Masks off priorities that should not be scheduled default: 0 is masked
 *
 * @details Priority 0 (highest) should always be masked.
 *          Threads with that priority may not be programmed
 *          with the possibility of being scheduled in mind.
 *          Parts of this scheduler assume 0 current_rr_priority is uninitialised.
 */
#define SCHED_RR_MASK (1 << 0)
#endif

#include <inttypes.h>
#include <stdio.h>

#include "hardware.h"
#include "irq.h"
#include "msg.h"
#include "thread.h"


#define ENABLE_DEBUG 0
#include "debug.h"

void sched_round_robin_init(void);

static void _sched_round_robin_cb(void *d);

/*
 * Assuming simple reads from and writes to a byte to be atomic on every board
 * Value 0 is assumed to show this system is uninitialised.
 * The timer will not be started for prio = 0;
 */
static uint8_t _current_rr_priority = 0;

void sched_runq_callback(uint8_t prio);

void _sched_round_robin_cb(void *d)
{
    (void)d;
    /*
     * reorder current Round Robin priority
     * (put the current thread at the end of the run queue of its priority)
     * and setup the scheduler to schedule when returning from the IRQ
     */
    uint8_t prio = _current_rr_priority;
    if (prio != 0xff) {
        sched_runq_advance(prio);
        _current_rr_priority = 0xff;
    }
    thread_t *active_thread = thread_get_active();
    if (active_thread) {
        uint8_t active_priority = active_thread->priority;
        if (active_priority == prio) {
            thread_yield_higher();
            /* thread change will call the runqueue_change_cb */
        }
        else {
            sched_runq_callback(active_priority);
        }
    }
}

static inline void _sched_round_robin_remove(void)
{
    _current_rr_priority = 0xff;

}

static inline void _sched_round_robin_set(uint8_t prio)
{
    if (prio == 0) {
        return;
    }
    _current_rr_priority = prio;

}

void sched_runq_callback(uint8_t prio)
{
    if (SCHED_RR_MASK & (1 << prio) || prio == 0) {
        return;
    }

    if (_current_rr_priority == prio) {
        if (sched_runq_is_empty(prio)) {
            _sched_round_robin_remove();
            thread_t *active_thread = thread_get_active();
            if (active_thread) {
                prio = active_thread->priority;
            }
            else {
                return;
            }
        }
    }

    if (_current_rr_priority == 0xff  &&
        !(SCHED_RR_MASK & (1 << prio)) &&
        sched_runq_more_than_one(prio)) {
        _sched_round_robin_set(prio);
    }
}

void sched_round_robin_init(void)
{
    /* init _current_rr_priority */
    _current_rr_priority = 0xff;
    /* check if applicable to active priority */
    thread_t *active_thread = thread_get_active();
    if (active_thread) {
        sched_runq_callback(active_thread->priority);
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

__attribute__((section (".riot_body"), interrupt TIMERA0_VECTOR)) void TIMER_ISR(void)
{
        _sched_round_robin_cb(NULL);

        CCTL0 &= ~CCIFG;
}

static char stack[3][THREAD_STACKSIZE_DEFAULT];


static kernel_pid_t main_pid;

static const uint8_t shared_prio = THREAD_PRIORITY_MAIN + 1;

void * thread_wakeup_main(void *d)
{
    (void) d;
    P1OUT = 0x01; 
    thread_yield();
    int x = 0;
    while ((x < 10) && (thread_wakeup(main_pid) == (int)STATUS_NOT_FOUND)) {
        thread_yield();
        x++;
    };
    P1OUT = 0x02;
    return NULL;
}

void * thread_bad(void *d)
{
    (void) d;
    P1OUT = 0x03; 
    for (;;) {
        /* I'm a bad thread I do nothing and I do that all the time */
    }
}

void * main_thread(void *d) {
    (void) d; 

    main_pid = thread_getpid();

    thread_create(stack[0], sizeof(stack[0]), shared_prio, 0,
                  thread_wakeup_main, NULL, "TWakeup");
    thread_create(stack[1], sizeof(stack[1]), shared_prio, 0,
                  thread_bad, NULL, "TBad");

    P3OUT = 0x08; 
    thread_sleep(); 


    P3OUT = 0x09; 

    while (1) {

    }
}

int main (void) {
    setup();
        __enable_interrupt();

    thread_create(stack[2], sizeof(stack[2]), THREAD_PRIORITY_MAIN, 0, main_thread, NULL, "main");
}