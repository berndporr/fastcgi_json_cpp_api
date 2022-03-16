/*
 * Copyright (c) 2013-2021  Bernd Porr <mail@berndporr.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

#include <string.h>

#include "json_fastcgi_web_api.h"
#include "fakesensor.h"

/**
 * Flag to indicate that we are running.
 * Needed later to quit the idle loop.
 **/
int mainRunning = 1;

/**
 * Handler when the user has pressed ctrl-C
 * send HUP via the kill command.
 **/
void sigHandler(int sig) { 
	if((sig == SIGHUP) || (sig == SIGINT)) {
		mainRunning = 0;
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
 * Handler which receives the data here just saves
 * the most recent sample with timestamp. Obviously,
 * in a real application the data would be stored
 * in a database and/or triggers events and other things!
 **/
class SENSORfastcgicallback : public SensorCallback {
public:
	std::deque<float> temperatureBuffer;
	std::deque<long> timeBuffer;
	long t;
	const int maxBufSize = 10;

	/**
	 * Callback with the fresh ADC data.
	 * That's where all the internal processing
	 * of the data is happening. Here, we just
	 * convert the raw ADC data to temperature
	 * and store it in a variable.
	 **/
	virtual void hasSample(int v) {
		temperatureBuffer.push_back(v);
		if (temperatureBuffer.size() > maxBufSize) temperatureBuffer.pop_front();
		// timestamp
		t = time(NULL);
		timeBuffer.push_back(t);
		if (timeBuffer.size() > maxBufSize) timeBuffer.pop_front();
	}

	void forceTemperature(float temp) {
		for(auto& v:temperatureBuffer) {
			v = temp;
		}
	}
};


/**
 * Callback handler which returns data to the
 * nginx server. Here, simply the current temperature
 * and the timestamp is transmitted to nginx and the
 * javascript application.
 **/
class JSONCGIADCCallback : public JSONCGIHandler::GETCallback {
private:
	/**
	 * Pointer to the ADC event handler because it keeps
	 * the data in this case. In a proper application
	 * that would be probably a database class or a
	 * controller keeping it all together.
	 **/
	SENSORfastcgicallback* sensorfastcgi;

public:
	/**
	 * Constructor: argument is the ADC callback handler
	 * which keeps the data as a simple example.
	 **/
	JSONCGIADCCallback(SENSORfastcgicallback* argSENSORfastcgi) {
		sensorfastcgi = argSENSORfastcgi;
	}

	/**
	 * Gets the data sends it to the webserver.
	 * The callback creates two json entries. One with the
	 * timestamp and one with the temperature from the sensor.
	 **/
	virtual std::string getJSONString() {
		JSONCGIHandler::JSONGenerator jsonGenerator;
		jsonGenerator.add("epoch",(long)time(NULL));
		jsonGenerator.add("temperature",sensorfastcgi->temperatureBuffer);
		jsonGenerator.add("time",sensorfastcgi->timeBuffer);
		return jsonGenerator.getJSON();
	}
};


/**
 * Callback handler which receives the JSON from jQuery
 **/
class SENSORPOSTCallback : public JSONCGIHandler::POSTCallback {
public:
	SENSORPOSTCallback(SENSORfastcgicallback* argSENSORfastcgi) {
		sensorfastcgi = argSENSORfastcgi;
	}

	/**
	 * As a crude example we force the temperature readings
	 * to be 20 degrees for a certain number of timesteps.
	 **/
	virtual void postString(std::string postArg) {
		auto m = JSONCGIHandler::postDecoder(postArg);
		float temp = atof(m["temperature"].c_str());
		std::cerr << m["hello"] << "\n";
		sensorfastcgi->forceTemperature(temp);
	}

	/**
	 * Pointer to the handler which keeps the temperature
	 **/
	SENSORfastcgicallback* sensorfastcgi;
};
	

// Main program
int main(int argc, char *argv[]) {
	// getting all the ADC related acquistion set up
	FakeSensor* sensorcomm = new FakeSensor();
	SENSORfastcgicallback sensorfastcgicallback;
	sensorcomm->setCallback(&sensorfastcgicallback);

	// Setting up the JSONCGI communication
	// The callback which is called when fastCGI needs data
	// gets a pointer to the SENSOR callback class which
	// contains the samples. Remember this is just a simple
	// example to have access to some data.
	JSONCGIADCCallback fastCGIADCCallback(&sensorfastcgicallback);

	// Callback handler for data which arrives from the the
	// browser via jquery json post requests:
	// $.post( 
        //              "/sensor/:80",
        //              {
	//		  temperature: [20,18,19,20],
	//                time: [171717,171718,171719,171720],
	//                hello: "Hello, that's a test!"
	//	      }
	//	  );
	SENSORPOSTCallback postCallback(&sensorfastcgicallback);
	
	// starting the fastCGI handler with the callback and the
	// socket for nginx.
	JSONCGIHandler* fastCGIHandler = new JSONCGIHandler(&fastCGIADCCallback,
							    &postCallback,
							    "/tmp/sensorsocket");

	// starting the data acquisition at the given sampling rate
	sensorcomm->startSensor();

	// catching Ctrl-C or kill -HUP so that we can terminate properly
	setHUPHandler();

	fprintf(stderr,"'%s' up and running.\n",argv[0]);

	// Just do nothing here and sleep. It's all dealt with in threads!
	// At this point for example a GUI could be started such as QT
	// Here, we just wait till the user presses ctrl-c which then
	// sets mainRunning to zero.
	while (mainRunning) sleep(1);

	fprintf(stderr,"'%s' shutting down.\n",argv[0]);

	// stopping ADC
	delete sensorcomm;

	// stops the fast CGI handlder
	delete fastCGIHandler;
	
	return 0;
}
