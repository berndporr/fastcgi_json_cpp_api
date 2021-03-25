# jQuery <--> C++ fastcgi web API

C++ Header-only event driven communication between jQuery in a web-browser via nginx.

This was developed because of a lack of a lightweight jQuery to C++
communication. It's a small helper which can easily be
included in any C++ application which needs to talk to a web page
where realtime data needs to be exchanged.

![alt tag](dataflow.png)

## Prerequisites

```
apt-get install libfcgi-dev
apt-get install libcurl4-openssl-dev
```

## Howto

The only file you need is:
```
json_fastcgi_web_api.h
```
Copy this header file into your project, include it and then overload the
abstract callbacks in the class.

The file `json_fastcgi_web_api.h` has extensive inline documentation. 
Its doxygen generated online documentation is here: 
https://berndporr.github.io/json_fastcgi_web_api/

### Implement the GET callback

This is the callback which sends JSON to the website:

```
	class GETCallback {
	public:
		/**
		 * Needs to return the payload data sent to the web browser.
		 * Use the JSON generator class or an external json generator.
		 **/
		virtual std::string getJSONString() = 0;
	};
```
Overload `getJSONString()` and return JSON. You can use the
class `JSONGenerator` to generate the JSON data: Use the `add`
methods to add key/value pairs and then get the json with the
method `getJSON()`.

### Implement the POST callback (optional)

This handler receives the JSON from jQuery POST command from the
website for example a button press. Implement the callback:

```
	class POSTCallback {
	public:
		/**
		 * Receives the data from the web browser in JSON format.
		 * Use postDecoder() to decode the JSON or use an external
		 * library.
		 **/
		virtual void postString(std::string arg) = 0;
	};
```
Overload `postString(std::string arg)` with a function
which decodes the received POST data.
You can use `postDecoder(std::string s)` which returns a
`std::map` of key/value pairs.

### Start the communication

The constructor takes as arguments the GET callback, the POST callback
and the path to the fastCGI socket. As soon as the constructor is
instantiated the communication starts.

```
       /**
         * Constructor which inits it and starts the main thread.
         * Provide an instance of the callback handler which provides the
         * payload data in return. The optional socketpath variable
         * can be set to another path for the socket which talks to the
         * webserver. postCallback is the callback which returns
         * received json packets as a map.
         **/
        JSONCGIHandler(GETCallback* argGetCallback,
                       POSTCallback* argPostCallback = nullptr,
		       const char socketpath[] = "/tmp/fastcgisocket");
```

### Stop the communication

This is done by deleting the instance.


## Example code

The `demo_sensor_server` fakes a temperature sensor
and this is plotted on the screen. The nginx
config file and the website are in the `website`
directory.

Start `demo_sensor_server`
in the background with:
```
nohup ./demo_sensor_server &
```
which creates a socket under `/tmp/sensorsocket` to communicate with
the fastcgi server.

### Configuring the nginx for FastCGI

 1. copy the the nginx config file `website/nginx-sites-enabled-default` to your
    nginx config directory `/etc/nginx/sites-enabled/default`.
 2. copy `website/fakesensor.html` to `/var/www/html`.
 
Then point your web-browser to `fakesensor.html` on your website.
You should see a fake
temperatue reading on the screen and a plot with dygraph.
The JSON packets can be viewed by appending `/sensor/` to the server URL.

The script sends also a JSON packet to the demo server which
requests to clamp the temperature to 20C and prints out a string
to stderr.

![alt tag](screenshot.png)
