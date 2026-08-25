#include <stdio.h>
#include "hardware.h"
#include "omsp_system.h"


// Watchdog timer
#define WDTCTL_              0x0120    /* Watchdog Timer Control */
#define WDTHOLD             (0x0080)
#define WDTPW               (0x5A00)

#define MAX_TASKS  4
#define TASK_BOUNDS_BYTES  MAX_TASKS*4
#define VIOLATE_FLAG_ADDR   0x400
#define VIOLATE_ID_ADDR     VIOLATE_FLAG_ADDR+2 //x402
#define QUEUE_ADDR          VIOLATE_ID_ADDR+2   //x404
#define BOUNDS_ADDR         0x200 //QUEUE_ADDR + 32     //x424

typedef void (*task_t)(void);
task_t * scheduled_tasks = (task_t*)(QUEUE_ADDR);

int shared_int = 1;
int total_scheduled = 0;

void func1(){
    delay(shared_int);
    shared_int = shared_int + 2;
    delay(shared_int);
}

void func2(){
    delay(shared_int);
    shared_int = shared_int << 2;
    delay(shared_int);
}

void func3(){
    delay(shared_int);
    shared_int = shared_int - 2;
    delay(shared_int);
}

void func4(){
    delay(shared_int);
    shared_int = shared_int >> 2;
    delay(shared_int);
}

__attribute__ ((section (".riot_body"))) void delay(int ticks){
    for(int i=0; i<ticks; i++);
}

__attribute__ ((section (".riot_body"))) void register_task(task_t t){
    if(total_scheduled < MAX_TASKS){
        scheduled_tasks[total_scheduled] = t;
        total_scheduled++;
    }
}

__attribute__ ((section (".riot_body"))) void naive_scheduler(){
    task_t t;
    for(int i=0; i<total_scheduled; i++){
        t = scheduled_tasks[i];
        t();
    }
}

__attribute__ ((section (".riot_body"))) void setup(){
    uint32_t* wdt = (uint32_t*)(WDTCTL_);
    *wdt = WDTPW | WDTHOLD;

    register_task(&func1);
    register_task(&func2);
    register_task(&func3);
    register_task(&func4);
    
    naive_scheduler();
}

// --------------------- Main ------------------//
int main(void)
{
    setup();
    
    // // test read and write of metadata region   
    // uint8_t * violate_flag_ptr = (uint8_t*)(VIOLATE_FLAG_ADDR);
    // uint8_t * violate_id_ptr = (uint8_t*)(VIOLATE_ID_ADDR);
    // *violate_flag_ptr = 0x11;
    // *violate_id_ptr = 0x22;
    // P3OUT = *violate_flag_ptr;
    // P3OUT = *violate_id_ptr;

    // uint8_t * task_bounds = (uint8_t*)(BOUNDS_ADDR);
    // for(int i=0; i<TASK_BOUNDS_BYTES; i++){
    //     P3OUT = task_bounds[i];
    // }

    LPM0;
}
