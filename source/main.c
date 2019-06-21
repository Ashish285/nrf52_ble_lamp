#include "gpio.h"
#include "bluetooth.h"
#include "pwm.h"
#include "main.h"
#include "app_fds.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "nrf_pwr_mgmt.h"
#include "rainbow.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/**************** Timing Variables ****************/
uint32_t ticks_to = 0;
uint32_t ticks_from = 0;
uint32_t timestamp_ms = 0;


#ifdef PWM_DEBUG
int red = 0;
int green = 0;
int blue = 0;
int alpha1 = 0;

bool up = false;
bool down = false;
#endif

/**************************Private Function Definitions**************************/

/**
* @brief                       Function to call the power management
* @param[in]                   NONE
* @retval                      NONE
*/
void idle_state_handle()
{
  nrf_pwr_mgmt_run();
}

/**
* @brief             Start the low frequency clock required for RTC
* @param[in]         NONE
* @retval            NONE
*/
void clock_start()
{
  NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_LFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
}

/**
* @brief             Gives the timestamp on the log
* @param[in]         NONE
* @retval            NONE
*/
uint32_t timestamp_func()
{
  uint32_t ticks_diff = 0;
  uint32_t ms_diff = 0;
  uint32_t ticks_to = app_timer_cnt_get();
  
  ticks_diff = app_timer_cnt_diff_compute(ticks_to, ticks_from);
  ticks_from = ticks_to;
  
  ms_diff = APP_TIMER_MS(ticks_diff, APP_TIMER_PRESCALER);
  
  timestamp_ms += ms_diff;
  
  return timestamp_ms;
}

/**
* @brief             Initialise Bluetooth
* @param[in]         NONE
* @retval            NONE
*/
void init_bluetooth()
{
 
  uint32_t err_code;
  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
  
  init_ble_stack();
  gap_params_init();
  gatt_init();
  services_init();
  init_advertising();
  conn_params_init();
  peer_manager_init();
  advertising_start();
}

/**
* @brief             Initialise the peripherals
* @param[in]         NONE
* @retval            NONE
*/
void init_peripherals()
{
  clock_start();
  
  APP_ERROR_CHECK(NRF_LOG_INIT(timestamp_func));
  
  NRF_LOG_DEFAULT_BACKENDS_INIT();                                                      //Use RTT as default backend
  NRF_LOG_INFO("Start\r\n");
  
  pwm_init();
  init_bluetooth();
  init_fds();
  
  config_leds();
}

/**
* @brief             Main Function
* @param[in]         NONE
* @retval            NONE
*/
int main()
{
  init_peripherals();
  
#ifdef PWM_DEBUG
  while(1)
  {
    pwm_set_duty_cycle(red, green, blue, alpha1);
    if(alpha == 10000)
    {
      down = 1;
      up = 0;
    }
    else if(alpha == 0)
    {
      up = 1;
      down = 0;
    }
    
    if(up)
    {
      red += 5;
      green += 10;
      blue += 15;
      alpha1 += 20;
      nrf_delay_ms(10);
    }
    else
    {
      red -= 5;
      green -= 10;
      blue -= 15;
      alpha1 -= 20;
      nrf_delay_ms(10);
    }
  }
#endif
  
  while(1)
  {
    idle_state_handle();
  }
}

