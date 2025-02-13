# jQuery <--> C++ JSON communication

C++ Header-only event driven communication between jQuery in a web-browser via the nginx web-server.
This can implement, for example, a RESTful API for fast sensor data transfer.

This was developed because of a lack of a lightweight jQuery to C++
communication. It's a small helper which can easily be
included in any C++ application which needs to talk to a web page
where realtime data needs to be exchanged.

Works alongside PHP as nginx can serve both!

![alt tag](dataflow.png)

## Prerequisites

```
apt-get install libfcgi-dev
apt-get install libjsoncpp-dev
apt-get install nginx-core
```

## Installation

This is a pure header based library. All the code is in `json_fastcgi_web_api.h`. Just type:
```
cmake .
sudo make install json_fastcgi_web_api.h
```
to install the header in the system-wide include dir.

## Howto

The only file you need is:
```
json_fastcgi_web_api.h
```

The file `json_fastcgi_web_api.h` has extensive inline documentation. 
Its doxygen generated online documentation is here: 
https://berndporr.github.io/fastcgi_json_cpp_api/

### Implement the GET callback (server -> client)

This is the callback which sends JSON packets to the client (website, phone app, etc):

```
	class GETCallback {
	public:
		/**
		 * Needs to return the JSON data sent to the web browser.
		 **/
		virtual std::string getJSONString() = 0;
	};
```
Overload `getJSONString()` and return JSON. The recommended way
of generating JSON is with the [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
library which is part of all major Linux distros.

### Implement the POST callback (client -> server, optional)

This handler receives the JSON from jQuery POST command from the
website for example when the user presses a button. Implement the callback:

```
	class POSTCallback {
	public:
		/**
		 * Receives the data from the web browser in JSON format.
		 **/
		virtual void postString(std::string arg) = 0;
	};
```
Overload `postString(std::string arg)` with a function which decodes the received POST data.

### Start the communication

The start method takes as arguments the GET callback, the POST callback
and the path to the fastCGI socket:

```
JSONCGIHandler jsoncgihandler;
jsoncgihandler.start(GETCallback* argGetCallback,
                     POSTCallback* argPostCallback = nullptr,
		     const char socketpath[] = "/tmp/fastcgisocket");
```

### Stop the communication

Just call `jsoncgihandler.stop()` to shut down the communication.


## Example code

The subdir `fake_sensor_demo` contains a `demo_sensor_server` which fakes a temperature sensor
and its readings are plotted in a web browser. The nginx
config file and the website are in the `website`
directory.



Bernd Porr, mail@berndporr.me.uk

## Dynamic DNS
if you have no static IP address you can use the the "afraid" dynamic DNS service:
[Free DNS](http://freedns.afraid.org/)
