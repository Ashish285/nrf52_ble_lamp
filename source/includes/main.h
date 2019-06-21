#ifndef MAIN_H__
#define MAIN_H__


#define APP_TIMER_MS(TICKS, PRESCALER)\
            ((uint64_t)(TICKS * ((PRESCALER + 1) * 1000)) / (uint64_t)APP_TIMER_CLOCK_FREQ)
            //((uint32_t)ROUNDED_DIV((TICKS) * ((PRESCALER) + 1) * 1000, (uint64_t)APP_TIMER_CLOCK_FREQ))
#define APP_TIMER_PRESCALER             APP_TIMER_CONFIG_RTC_FREQUENCY                                           /**< Value of the RTC1 PRESCALER register. */

              
#define WAKEUP_INTERVAL                     APP_TIMER_TICKS(DUTY_CYCLE)                    //1000ms*1 = 1s*60 = 60s/1m*5 = 5m
#define ADVERTISE_INTERVAL                  APP_TIMER_TICKS(advertise_duration)



/**************************Private Function Declarations**************************/
              
void start_advertise();

int get_ms_diff();

#endif //MAIN_H__