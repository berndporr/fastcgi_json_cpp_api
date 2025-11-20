#ifndef __DS18B20_H
#define __DS18B20_H

/**
 * Copyright (c) 2025  Bernd Porr <mail@berndporr.me.uk>
 **/

#include <math.h>
#include "CppTimer.h"

/**
 * Callback for new samples which needs to be implemented by the main program.
 * The function hasTemperature needs to be overloaded in the main program.
 **/
class SensorCallback {
public:
    /**
     * Called after a new tempertature reading has arrived
     **/
    virtual void hasTemperature(float degrees) = 0;
};


/**
 * This class reads at regular intervals the temperature from the DS18B20
 * and delivers the temperature readings via a callback interface.
 **/
class DS18B20 : public CppTimer {

public:

    /**
     * Default constructor
     **/
    DS18B20() = default;

    /**
     * Destructor stops acquision
     **/
    ~DS18B20() {
	stop();
    }

    /**
     * Sets the callback which is called whenever there is a sample
     **/
    void setCallback(SensorCallback* cb) {
	sensorCallback = cb;
    }

    /**
     * Starts the data acquision
     * \param sensorTmperaturePath Abs path to the temperature file of the sensor
     * \param samplingIntervalSec Sampling interval in seconds
     **/
    void start(std::string sensorTmperaturePath, int samplingIntervalSec = 10) {
	dsPath = sensorTmperaturePath;
	// dummy read to see if it works
	const float t = readSensor();
	fprintf(stderr,"Sensor read OK: %3.1fC. Measuring every %dsec.\n",
		t,samplingIntervalSec);
	startms(samplingIntervalSec*1000);
    }

private:
    /**
     * Reads temperature from the sensor
     **/
    float readSensor() {
	FILE* f = fopen(dsPath.c_str(),"rt");
	if (!f) {
	    fprintf(stderr,"Could not open %s.\n",dsPath.c_str());
	    exit(-1);
	}
	float value;
	const int r = fscanf(f,"%f",&value);
	fclose(f);
	if (r > 0) {
	    return value/1000.0f;
	} else {
	    fprintf(stderr,"Could not read from %s. Error code: %d.",dsPath.c_str(),r);
	    exit(-1);
	}
    }
    
    /**
     * Timer callback
     **/
    void timerEvent() override {
	if (nullptr == sensorCallback) return;
	const float temperature = readSensor();
	sensorCallback->hasTemperature(temperature);
    }


private:
    SensorCallback* sensorCallback = nullptr;
    std::string dsPath;
};


#endif
