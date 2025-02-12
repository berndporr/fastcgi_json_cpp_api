#ifndef __FAKESENSOR_H
#define __FAKESENSOR_H

/**
 * Copyright (c) 2025  Bernd Porr <mail@berndporr.me.uk>
 **/

#include <math.h>
#include <chrono>
#include <thread>
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
class FakeSensor : public CppTimer {

public:

    FakeSensor() = default;

    ~FakeSensor() {
	stop();
    }

    /**
     * Sets the callback which is called whenever there is a sample
     **/
    void setCallback(SensorCallback* cb) {
	sensorCallback = cb;
    }

    void start() {
	startms(100);
    }

private:
    /**
     * Fake the arrival of data
     **/
    void timerEvent() {
	float value = sin(t) * 5 + 20;
	t += 0.1;
	if (nullptr != sensorCallback) {
	    sensorCallback->hasSample(value);
	}
    }


private:
    SensorCallback* sensorCallback = nullptr;
    float t = 0;
    bool running = false;
    std::thread thr;
};


#endif
