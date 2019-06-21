# nrf52_ble_lamp
A simple BLE lamp which takes in values for red,green,blue and alpha and sets the LED color accordingly. The values are stored in the Flash and are restored on a restart.

# Logging
This project uses Segger RTT for logs. The Passkey for BLE connection will be displayed on RTT log.

# How to Use
Set the LED pins in the 'gpio.h' file inside the includes directory. The Alpha pin is the common pin, depending upon the LED, the common ground or the common power.

Flash the firmware on a nrf52 device. This was tested on nrf52 dev kit(nrf52-dk) but should work with other nrf52 devices.
Get the 'nrf Connect' app on your phone. The device will advertise with name 'Lamp'. You can change it by changing the "DEVICE_NAME' parameter in 'bluetooth.c' file.

Connect to the device, the Passkey will be displayed on the RTT logs. Pair with the device.

To set a color, the led PWM has to be set. From the phone, send a string  with format "RR,GG,BB,AA", where RR is a 0-99 value for the Red color. Similarly, 'GG' is for Green and 'BB'is for Blue color. 'AA' is the Alpha, the brightness.
