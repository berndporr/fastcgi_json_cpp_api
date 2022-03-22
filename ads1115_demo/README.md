# ADS1115 ADC demo

![alt tag](screenshot.png)

This is a demo with the real ADC ADS1115 which displays
channel 1 at a sampling rate of 8Hz on a website.

## Prerequisites

Install the ADS1115 class from here:
https://github.com/berndporr/rpi_ads1115

## Running the fast CGI server
The fast cgi server `ads1115_server` creates a socket under
`/tmp/sensorsocket` to communicate with nginx.

 1. For testing purposes in the foreground you can directly run the fastcgi server with
 ```
 ./ads1115_server
 ```

 2. For production use in the background run:
 ```
 nohup ./ads1115_server &
 ```

## Configuring the nginx for FastCGI

 1. copy the the nginx config file `website/nginx-sites-enabled-default` to your
    nginx config directory `/etc/nginx/sites-enabled/default`.
 2. copy `website/ads1115.html` to `/var/www/html`.
 
Then point your web-browser to `ads1115.html` on your website.
You should see the ADC readings on the screen and a plot with dygraph.
The JSON packets can be viewed by appending `/sensor/` to the server URL.

The script sends also a JSON packet to the demo server which
requests to clamp the ADC value to 1 and prints out a string
to stderr.

