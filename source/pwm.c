#include "nrf_drv_pwm.h"
#include "gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);
static uint16_t const pwm_top_val  = 10000;
static nrf_pwm_values_individual_t seq_values;
static nrf_pwm_sequence_t const    seq =
{
    .values.p_individual = &seq_values,
    .length              = NRF_PWM_VALUES_LENGTH(seq_values),
    .repeats             = 0,
    .end_delay           = 0
};


/**
* @brief             Function to convert duty cycle values to raw value to update the PWM compare register
* @param[in]         NONE
* @retval            NONE
*/
int convert_duty_to_raw(int duty)
{
  return (((duty*pwm_top_val)/100));
}

/**
* @brief             Sets the PWM according to the duty cycle
* @param[in]         red_duty           Duty cycle for red led
* @param[in]         green_duty         Duty cycle for green led
* @param[in]         blue_duty          Duty cycle for blue led
* @param[in]         alpha_duty         Duty cycle for alpha i.e. brightness
* @retval            NONE
*/
void pwm_set_duty_cycle(int red_duty, int green_duty, int blue_duty, int alpha_duty)
{
  int raw_red = 0;
  int raw_green = 0;
  int raw_blue = 0;
  int raw_alpha = 0;
  
  raw_red = convert_duty_to_raw(red_duty);
  raw_green = convert_duty_to_raw(green_duty);
  raw_blue = convert_duty_to_raw(blue_duty);
  raw_alpha = convert_duty_to_raw(alpha_duty);
 /* 
  NRF_LOG_INFO("Red Duty: %d Red Raw: %d",red_duty, raw_red);
  NRF_LOG_INFO("Green Duty: %d Green Raw: %d",green_duty, raw_green);
  NRF_LOG_INFO("Blue Duty: %d Blue Raw: %d",blue_duty, raw_blue);
  NRF_LOG_INFO("Alpha Duty: %d Alpha Raw: %d",alpha_duty, raw_alpha);
  */
  
  seq_values.channel_0 = raw_red;
  seq_values.channel_1 = raw_green;
  seq_values.channel_2 = raw_blue;
  seq_values.channel_3 = raw_alpha;
  
  //nrf_drv_pwm_simple_playback(&m_pwm0, &seq, 1, NRF_DRV_PWM_FLAG_LOOP);
}


/**
* @brief             Function to initialise PWM and start with 0 duty cycle
* @param[in]         NONE
* @retval            NONE
*/
void pwm_init()
{
  nrf_drv_pwm_config_t const config0 =
  {
    .output_pins =
    {
      red_led,
      green_led,
      blue_led,
      alpha
    },
    .irq_priority = APP_IRQ_PRIORITY_LOWEST,
    .base_clock   = NRF_PWM_CLK_1MHz,
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = pwm_top_val,
    .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
    .step_mode    = NRF_PWM_STEP_AUTO
  };
  
  APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, NULL));
  
  seq_values.channel_0 = 0;
  seq_values.channel_1 = 0;
  seq_values.channel_2 = 0;
  seq_values.channel_3 = 0;
  
  (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq, 1,
                                      NRF_DRV_PWM_FLAG_LOOP);
}