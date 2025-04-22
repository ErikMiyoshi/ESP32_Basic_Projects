Example using adc + wifi + mqtt + docker (with mosquitto + telegraf + influxdb).

For every period of time, it read the value on the ADC, and send data over mqtt topic.

Configure the URL of broker in menuconfig, also configure the credentials for wifi connection.