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
 * This class reads data from a fake sensor in the background
 * and calls a callback function whenever data is available.
 **/
class DS18B20 : public CppTimer {

public:

    DS18B20() = default;

    ~DS18B20() {
	stop();
    }

    /**
     * Sets the callback which is called whenever there is a sample
     **/
    void setCallback(SensorCallback* cb) {
	sensorCallback = cb;
    }

    void start(std::string sensorTmperaturePath, int samplingIntervalSec = 10) {
	dsPath = sensorTmperaturePath;
	startms(samplingIntervalSec*1000);
    }

private:
    /**
     * The arrival of data
     **/
    void timerEvent() {
	if (nullptr == sensorCallback) return;
	FILE* f = fopen(dsPath.c_str(),"rt");
	if (!f) {
	    fprintf(stderr,"Could not open %s.\n",dsPath.c_str());
	    return;
	}
	float value;
	const int r = fscanf(f,"%f",&value);
	if (r > 0) {
	    sensorCallback->hasTemperature(round(value/100.0f)/10.0f);
	} else {
	    fprintf(stderr,"Could not read from %s. Error code: %d.",dsPath.c_str(),r);
	}
	fclose(f);
    }


private:
    SensorCallback* sensorCallback = nullptr;
    std::string dsPath;
};


#endif
