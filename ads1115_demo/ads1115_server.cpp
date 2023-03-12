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

#include "json_fastcgi_web_api.h"
#include "ads1115rpi.h"
#include <json/json.h>

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
class ADS1115datasink : public ADS1115rpi {
public:
	std::deque<float> values;
	const int maxBufSize = 50; 

	/**
	 * Callback with the fresh ADC data.
	 * That's where all the internal processing
	 * of the data is happening.
	 **/
	virtual void hasSample(float v) {
		values.push_back(v);
		if (values.size() > maxBufSize) values.pop_front();
	}

	void forceValue(float a) {
		for(auto& v:values) {
			v = a;
		}
	}
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
	ADS1115datasink* sensorfastcgi;

public:
	/**
	 * Constructor: argument is the ADC callback handler
	 * which keeps the data as a simple example.
	 **/
	JSONCGIADCCallback(ADS1115datasink* argSENSORfastcgi) {
		sensorfastcgi = argSENSORfastcgi;
	}

	/**
	 * Gets the data sends it to the webserver -> client.
	 **/
	virtual std::string getJSONString() {
    	Json::Value root;
        root["epoch"] = (long)time(NULL);
        Json::Value values;
        for(int i = 0; i < sensorfastcgi->values.size(); i++) {
        	values[i] = sensorfastcgi->values[i];
    	}
        root["values"]  = values;
    	root["fs"] = (float)(sensorfastcgi->getADS1115settings().getSamplingRate());
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
	SENSORPOSTCallback(ADS1115datasink* argSENSORfastcgi) {
		sensorfastcgi = argSENSORfastcgi;
	}

	/**
	 * As a demo we force the voltage readings in the buffer to
	 * a certain value.
	 **/
	virtual void postString(std::string postArg) {
		// Let's parse the JSON
		const auto rawJsonLength = static_cast<int>(postArg.length());
		JSONCPP_STRING err;
		Json::Value root;
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		if (!reader->parse(postArg.c_str(), postArg.c_str() + rawJsonLength, &root,
				   &err)) {
			std::cout << "error" << std::endl;
			return;
		}
		// JSON now parsed into a std::map
		float temp = root["volt"].asFloat();
		std::cerr << root["hello"].asString() << "\n";
		sensorfastcgi->forceValue(temp);
	}

	/**
	 * Pointer to the handler which keeps the adc values
	 **/
	ADS1115datasink* sensorfastcgi;
};
	

// Main program
int main(int argc, char *argv[]) {
	// getting all the ADC related acquistion set up
        ADS1115datasink sensorcomm;

	// Setting up the JSONCGI communication
	// The callback which is called when fastCGI needs data
	JSONCGIADCCallback fastCGIADCCallback(&sensorcomm);

	// Callback handler for data which arrives from the
	// browser via jquery json post requests
	SENSORPOSTCallback postCallback(&sensorcomm);

	// instance of our fastCGI handler
	JSONCGIHandler fastCGIHandler;

	// starting the fastCGI handler with the callback and the
	// socket for nginx.
	fastCGIHandler.start(&fastCGIADCCallback,
			     &postCallback,
			     "/tmp/sensorsocket");

	ADS1115settings s;
        s.samplingRate = ADS1115settings::FS8HZ;

	// starting the data acquisition at the given sampling rate
	sensorcomm.start(s);

	// catching Ctrl-C or kill -HUP so that we can terminate properly
	setHUPHandler();

	fprintf(stderr,"'%s' up and running.\n",argv[0]);

	// Just do nothing here and sleep. It's all dealt with in threads!
	// At this point for example a GUI could be started such as QT
	// Here, we just wait till the user presses ctrl-c which then
	// sets mainRunning to zero.
	while (mainRunning) sleep(1);

	// stopping the data acquisition
	sensorcomm.stop();

	fprintf(stderr,"'%s' shutting down.\n",argv[0]);

	// stops the fast CGI handlder
	fastCGIHandler.stop();
	
	return 0;
}
