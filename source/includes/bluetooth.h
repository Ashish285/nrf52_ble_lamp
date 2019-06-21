#ifndef BLE_H__
#define BLE_H__


/**************************Private Function Declarations**************************/

void init_ble_stack();

void gap_params_init();

void init_gatt();

void init_advertising();

void gatt_init();

void services_init();

void conn_params_init();

void advertising_start();

void advertising_start();

void stop_advertise();

void peer_manager_init();

void send_data_ble(unsigned char *,unsigned int );

void disconnect();

void ble_timer_init();

void parse_ble_data();

#endif //BLE_H__
