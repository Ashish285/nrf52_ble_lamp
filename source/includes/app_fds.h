#ifndef APP_FDS_H__
#define APP_FDS_H__

#define PWM_FILE     (0xF010)
#define PWM_REC_KEY  (0x7010)

void init_fds();

void update_record_in_flash(char *ble_data);

#endif //APP_FDS_H__
