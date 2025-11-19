#ifndef __FAKESENSOR_H
#define __FAKESENSOR_H

/**
 * Copyright (c) 2025  Bernd Porr <mail@berndporr.me.uk>
 **/

#include <math.h>
#include "CppTimer.h"

/**
 * Callback for new samples which needs to be implemented by the main program.
 * The function hasSample needs to be overloaded in the main program.
 **/
class SensorCallback {
public:
    /**
     * Called after a sample has arrived.
     **/
    virtual void hasSample(float sample) = 0;
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

    void start(std::string sensorTmperaturePath) {
	dsPath = sensorTmperaturePath;
	startms(10000);
    }

private:
    /**
     * The arrival of data
     **/
    void timerEvent() {
	FILE* f = fopen(dsPath.c_str(),"rt");
	float value;
	fscanf(f,"%f",&value);
	if (nullptr != sensorCallback) {
	    sensorCallback->hasSample(value/1000.0f);
	}
	fclose(f);
    }


private:
    SensorCallback* sensorCallback = nullptr;
    std::string dsPath;
};


#endif
