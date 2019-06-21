#ifndef PWM_H__
#define PWM_H__

void pwm_init();

void pwm_set_duty_cycle(int red_duty, int green_duty, int blue_duty, int alpha_duty);


#endif //PWM_H__