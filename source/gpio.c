#include "gpio.h"
#include "nrf_gpio.h"

/**************************Private Function Definitions**************************/

/**
* @brief             Turn all LED off at the start
* @param[in]         NONE
* @retval            NONE
*/
void all_led_off()
{
  nrf_gpio_pin_set(LED1);
  nrf_gpio_pin_set(LED2);
  nrf_gpio_pin_set(LED3);
}

void led1_on()
{
  nrf_gpio_pin_clear(LED1);
}

void led2_on()
{
  nrf_gpio_pin_clear(LED2);
}

void led1_off()
{
  nrf_gpio_pin_set(LED1);  
}

void led2_off()
{
  nrf_gpio_pin_set(LED2);  
}



/**
* @brief             Configures the LED pins as output
* @param[in]         NONE
* @retval            NONE
*/
void config_leds()
{
  nrf_gpio_cfg_output(LED1);
  nrf_gpio_cfg_output(LED2);
  nrf_gpio_cfg_output(LED3);
  
  all_led_off();
}
