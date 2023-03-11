
/*
 * Copyright (c) 2013-2023  Bernd Porr <mail@berndporr.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

#include <string.h>
#include <unistd.h>
#include <json/json.h>

#include "json_fastcgi_web_api.h"
#include "cpp-usbdux.h"

/**
 * Flag to indicate that we are running.
 * Needed later to quit the idle loop.
 **/
bool mainRunning = true;

/**
 * Handler when the user has pressed ctrl-C
 * send HUP via the kill command.
 **/
void sigHandler(int sig) { 
	if((sig == SIGHUP) || (sig == SIGINT)) {
		mainRunning = false;
	}
}


/** 
 * Sets a signal handler so that you can kill
 * the background process gracefully with:
 * kill -HUP <PID>
 **/
void setHUPHandler() {
	struct sigaction act;
	memset (&act, 0, sizeof (act));
	act.sa_handler = sigHandler;
	if (sigaction (SIGHUP, &act, NULL) < 0) {
		perror ("sigaction");
		exit (-1);
	}
	if (sigaction (SIGINT, &act, NULL) < 0) {
		perror ("sigaction");
		exit (-1);
	}
}


/**
 * ADC class which receives the data
 **/
class USBDUXdatasink : public CppUSBDUX::Callback {
public:
	std::deque<float> values;
	const int maxBufSize = 50; 

	/**
	 * Callback with the fresh ADC data.
	 * That's where all the internal processing
	 * of the data is happening.
	 **/
	virtual void hasSample(const std::vector<float> &data) {
		const float scaling = 100;
		values.push_back(data[0] * scaling);
		if (values.size() > maxBufSize) values.pop_front();
	}

	void forceValue(float a) {
		for(auto& v:values) {
			v = a;
		}
	}

	float fs = 0;
};


/**
 * Callback handler which returns data to the
 * nginx server.
 **/
class JSONCGIADCCallback : public JSONCGIHandler::GETCallback {
private:
	/**
	 * Pointer to the ADC event handler because it keeps
	 * the data in this case. In a proper application
	 * that would be probably a database class or a
	 * controller keeping it all together.
	 **/
	USBDUXdatasink* datasink;

public:
	/**
	 * Constructor: argument is the ADC callback handler
	 * which keeps the data as a simple example.
	 **/
	JSONCGIADCCallback(USBDUXdatasink* argSENSORfastcgi) {
		datasink = argSENSORfastcgi;
	}

	/**
	 * Gets the data sends it to the webserver -> client.
	 **/
	virtual std::string getJSONString() {
		Json::Value root;
		root["epoch"] = (long)time(NULL);
		Json::Value values;
		for(int i = 0; i < datasink->values.size(); i++) {
			values[i] = datasink->values[i];
		}
		root["values"]  = values;
		root["fs"] = datasink->fs;
		Json::StreamWriterBuilder builder;
  		const std::string json_file = Json::writeString(builder, root);
		return json_file;
	}
};


/**
 * Callback handler which receives the JSON from jQuery
 **/
class SENSORPOSTCallback : public JSONCGIHandler::POSTCallback {
public:
	SENSORPOSTCallback(USBDUXdatasink* argSENSORfastcgi) {
		datasink = argSENSORfastcgi;
	}

	/**
	 * We force the voltage readings in the buffer to
	 * a certain value.
	 **/
	virtual void postString(std::string postArg) {
		auto m = JSONCGIHandler::postDecoder(postArg);
		float temp = atof(m["degrees"].c_str());
		std::cerr << m["hello"] << "\n";
		datasink->forceValue(temp);
	}

	/**
	 * Pointer to the handler which keeps the adc values
	 **/
	USBDUXdatasink* datasink;
};
	

// Main program
int main(int argc, char *argv[]) {
	// getting all the ADC related acquistion set up
    USBDUXdatasink datasink;

	// Setting up the JSONCGI communication
	// The callback which is called when fastCGI needs data
	JSONCGIADCCallback fastCGIADCCallback(&datasink);

	// Callback handler for data which arrives from the
	// browser via jquery json post requests
	SENSORPOSTCallback postCallback(&datasink);
	
	// starting the fastCGI handler with the callback and the
	// socket for nginx.
	JSONCGIHandler* fastCGIHandler = new JSONCGIHandler(&fastCGIADCCallback,
							    &postCallback,
							    "/tmp/sensorsocket");

	CppUSBDUX cppUSBDUX;
	cppUSBDUX.open();
	// starting the data acquisition at the given sampling rate
	try {
		cppUSBDUX.start(&datasink,16,10);
	} catch (const char* c) {
		fprintf(stderr,"%s\n",c);
		exit(-1);
	}

	datasink.fs = cppUSBDUX.getSamplingRate();
	fprintf(stderr,"fs = %f Hz",datasink.fs);

	// catching Ctrl-C or kill -HUP so that we can terminate properly
	setHUPHandler();

	fprintf(stderr,"'%s' up and running.\n",argv[0]);

	// Just do nothing here and sleep. It's all dealt with in threads!
	// At this point for example a GUI could be started such as QT
	// Here, we just wait till the user presses ctrl-c which then
	// sets mainRunning to zero.
	while (mainRunning) sleep(1);

	// stopping the data acquisition
	cppUSBDUX.stop();

	fprintf(stderr,"'%s' shutting down.\n",argv[0]);

	// stops the fast CGI handlder
	delete fastCGIHandler;
	
	return 0;
}
