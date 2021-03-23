/*
 * AD7705 test/demo program for the Raspberry PI
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2013-2021  Bernd Porr <mail@berndporr.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

#include "AD7705Comm.h"
#include "json_fastcgi_web_api.h"



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
class AD7705fastcgicallback : public AD7705callback {
public:
	float currentTemperature;
	float forcedTemperature;
	int forcedCounter = 0;
	long t;

	/**
	 * Callback with the fresh ADC data.
	 * That's where all the internal processing
	 * of the data is happening. Here, we just
	 * convert the raw ADC data to temperature
	 * and store it in a variable.
	 **/
	virtual void hasSample(int v) {
		// crude conversion to temperature
		currentTemperature = (float)v / 65536 * 2.5 * 0.6 * 100;
		if (forcedCounter > 0) {
			forcedCounter--;
			currentTemperature = forcedTemperature;
		}
		// timestamp
		t = time(NULL);
	}

	void forceTemperature(float temp, int nSteps) {
		forcedTemperature = temp;
		forcedCounter = nSteps;
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
	AD7705fastcgicallback* ad7705fastcgi;

public:
	/**
	 * Constructor: argument is the ADC callback handler
	 * which keeps the data as a simple example.
	 **/
	JSONCGIADCCallback(AD7705fastcgicallback* argAD7705fastcgi) {
		ad7705fastcgi = argAD7705fastcgi;
	}

	/**
	 * Gets the data sends it to the webserver.
	 * The callback creates two json entries. One with the
	 * timestamp and one with the temperature from the sensor.
	 **/
	virtual std::string getJSONString() {
		JSONCGIHandler::JSONGenerator jsonGenerator;
		jsonGenerator.add("epoch",(long)time(NULL));
		jsonGenerator.add("temperature",ad7705fastcgi->currentTemperature);
		return jsonGenerator.getJSON();
	}
};


/**
 * Callback handler which receives the JSON from jquery
 **/
class AD7705POSTCallback : public JSONCGIHandler::POSTCallback {
public:
	AD7705POSTCallback(AD7705fastcgicallback* argAD7705fastcgi) {
		ad7705fastcgi = argAD7705fastcgi;
	}

	/**
	 * As a crude example we force the temperature readings
	 * to be 20 degrees for a certain number of timesteps.
	 **/
	virtual void postJSONString(std::string json) {
		auto m = JSONCGIHandler::jsonDecoder(json);
		float temp = atof(m["temperature"].c_str());
		int steps = atoi(m["steps"].c_str());
		ad7705fastcgi->forceTemperature(temp,steps);
	}

	/**
	 * Pointer to the handler which keeps the temperature
	 **/
	AD7705fastcgicallback* ad7705fastcgi;
};
	

// Main program
int main(int argc, char *argv[]) {
	// getting all the ADC related acquistion set up
	AD7705Comm* ad7705comm = new AD7705Comm();
	AD7705fastcgicallback ad7705fastcgicallback;
	ad7705comm->setCallback(&ad7705fastcgicallback);

	// Setting up the JSONCGI communication
	// The callback which is called when fastCGI needs data
	// gets a pointer to the AD7705 callback class which
	// contains the samples. Remember this is just a simple
	// example to have access to some data.
	JSONCGIADCCallback fastCGIADCCallback(&ad7705fastcgicallback);

	// Callback handler for data which arrives from the the
	// browser via jquery json post requests:
	// $.post( 
        //              "/data/:80",
        //              {
	//		  temperature: 20,
	//		  steps: 100
	//	      }
	//	  );
	AD7705POSTCallback postCallback(&ad7705fastcgicallback);
	
	// starting the fastCGI handler with the callback and the
	// socket for nginx.
	JSONCGIHandler* fastCGIHandler = new JSONCGIHandler(&fastCGIADCCallback,
							    "/tmp/adc7705socket",
							    &postCallback);

	// starting the data acquisition at the given sampling rate
	ad7705comm->start(AD7705Comm::SAMPLING_RATE_50HZ);

	// catching Ctrl-C or kill -HUP so that we can terminate properly
	setHUPHandler();

	fprintf(stderr,"'%s' up and running.\n",argv[0]);

	// Just do nothing here and sleep. It's all dealt with in threads!
	// At this point for example a GUI could be started such as QT
	// Here, we just wait till the user presses ctrl-c.
	while (mainRunning) sleep(1);

	fprintf(stderr,"'%s' shutting down.\n",argv[0]);

	// stopping ADC
	delete ad7705comm;

	// stops the fast CGI handlder
	delete fastCGIHandler;
	
	return 0;
}
