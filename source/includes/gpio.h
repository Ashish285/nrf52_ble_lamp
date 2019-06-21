#ifndef GPIO_H__
#define GPIO_H__

//#define PWM_DEBUG
//#define PWM_DEBUG_PINS


#define LED1            17
#define LED2            18
#define LED3            19
#define LED4            20


#define red_led         27
#define green_led       26
#define blue_led        02
#define alpha           25

void config_leds();

void led1_on();

void led1_off();

void led2_on();

void led2_off();

#endif //GPIO_H__