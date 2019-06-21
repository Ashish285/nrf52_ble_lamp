#include "pwm.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

int red_duty_bow = 0;
int blue_duty_bow = 0;
int green_duty_bow = 0;
int alpha_duty_bow = 0;

char colors[][12] = 
{
  "30,00,80,00",        //violet
  "25,00,80,00",
  "20,00,80,00",
  "15,00,80,00",
  "10,00,80,00",        //indigo
  "05,00,80,00",
  "00,00,80,00",        //blue
  "00,10,70,00",
  "00,20,60,00",
  "00,30,50,00",
  "00,40,40,00",
  "00,50,30,00",
  "00,60,20,00",
  "00,70,10,00",
  "00,80,00,00",        //green
  "10,80,00,00",
  "20,80,00,00",
  "30,80,00,00",        //yellow
  "40,80,00,00",
  "40,70,00,00",
  "40,60,00,00",
  "50,60,00,00",
  "60,60,00,00",        //orange
  "60,50,00,00",
  "60,40,00,00",
  "60,30,00,00",
  "60,20,00,00",
  "60,10,00,00",
  "60,00,00,00"         //red
};

void parse_color(char *color)
{
  //NRF_LOG_INFO("Parsing %s",color);
  red_duty_bow = ((color[0] - '0')*10) + (color[1] - '0');
  green_duty_bow = ((color[3] - '0')*10) + (color[4] - '0');
  blue_duty_bow = ((color[6] - '0')*10) + (color[7] - '0');
  alpha_duty_bow = ((color[9] - '0')*10) + (color[10] - '0');
  
  pwm_set_duty_cycle(red_duty_bow, green_duty_bow, blue_duty_bow, alpha_duty_bow);
 // NRF_LOG_INFO("red: %d\tblue: %d\tgreen: %d",red_duty_bow,blue_duty_bow,green_duty_bow);
}

void do_rainbow()
{
  for(int i = 0; i < 29; i++)
  {
    NRF_LOG_INFO("Sending %s",colors[i]);
    parse_color(colors[i]);
    nrf_delay_ms(300);
  }
  
}
