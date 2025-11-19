/*
 * Copyright (c) 2013-2025
  Bernd Porr <mail@berndporr.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

#include <string.h>
#include <unistd.h>

#include "json_fastcgi_web_api.h"
#include "ds18b20.h"
#include <jsoncpp/json/json.h>

// Constants
const int temperatureBufferSize = 500;
const int samplingIntervalSec = 10;

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
    int maxBufSize;
    float lastValue = 0;
    
    SENSORfastcgicallback(int maxReadingsInBuffer) {
	maxBufSize = maxReadingsInBuffer;
    }
    
    /**
     * Callback with the fresh ADC data.
     * That's where all the internal processing
     * of the data is happening. Here, we just
     * convert the raw ADC data to temperature
     * and store it in a variable.
     **/
    virtual void hasTemperature(float v) {
	lastValue = v;
	temperatureBuffer.push_back(v);
	if (temperatureBuffer.size() > maxBufSize) temperatureBuffer.pop_front();
	// timestamp
	t = getTime();
	timeBuffer.push_back(t);
	if (timeBuffer.size() > maxBufSize) timeBuffer.pop_front();
    }

private:
    unsigned long getTime() const {
	std::chrono::time_point<std::chrono::system_clock> now = 
	    std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
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
        Json::Value root;
        root["epoch"] = (long)time(NULL);
	root["lastvalue"] = sensorfastcgi->lastValue;
        Json::Value temperature;
        for(int i = 0; i < sensorfastcgi->temperatureBuffer.size(); i++) {
        	temperature[i] = sensorfastcgi->temperatureBuffer[i];
    	}
        root["temperature"]  = temperature;
		Json::Value t;
        for(int i = 0; i < sensorfastcgi->timeBuffer.size(); i++) {
			t[i] = sensorfastcgi->timeBuffer[i];
    	}
		root["time"] = t;
        Json::StreamWriterBuilder builder;
    	const std::string json_file = Json::writeString(builder, root);
        return json_file;
	}
};


// Main program
int main(int argc, char *argv[]) {
    if (argc < 2) {
	fprintf(stderr,"Specify the path to the sensor. For example: /sys/bus/w1/devices/28-3ce1e380ac02/temperature.\n");
	exit(-1);
    }
    
    // getting all the ADC related acquistion set up
    DS18B20 sensorcomm;
    SENSORfastcgicallback sensorfastcgicallback(temperatureBufferSize);
    sensorcomm.setCallback(&sensorfastcgicallback);
    
    // Setting up the JSONCGI communication
    // The callback which is called when fastCGI needs data
    // gets a pointer to the SENSOR callback class which
    // contains the samples. Remember this is just a simple
    // example to have access to some data.
    JSONCGIADCCallback fastCGIADCCallback(&sensorfastcgicallback);
    
    // creating an instance of the fast CGI handler
    JSONCGIHandler jsoncgiHandler;
    
    // starting the fastCGI handler with the callback and the
    // socket for nginx.
    jsoncgiHandler.start(&fastCGIADCCallback,nullptr,
			 "/tmp/sensorsocket");
    
    // starting the data acquisition at the given sampling rate
    sensorcomm.start(argv[1]);
    
    // catching Ctrl-C or kill -HUP so that we can terminate properly
    setHUPHandler();
    
    fprintf(stderr,"'%s' up and running.\n",argv[0]);
    
    // Just do nothing here and sleep. It's all dealt with in threads!
    // At this point for example a GUI could be started such as QT
    // Here, we just wait till the user presses ctrl-c which then
    // sets mainRunning to zero.
    while (mainRunning) sleep(1);
    
    fprintf(stderr,"'%s' shutting down.\n",argv[0]);
    
    sensorcomm.stop();
    jsoncgiHandler.stop();
    
    return 0;
}
