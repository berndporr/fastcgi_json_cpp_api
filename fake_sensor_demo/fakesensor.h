#ifndef __FAKESENSOR_H
#define __FAKESENSOR_H

/**
 * Copyright (c) 2021  Bernd Porr <mail@berndporr.me.uk>
 **/

#include <math.h>
#include <chrono>
#include <thread>

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
class FakeSensor {

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

	/**
	 * Starts the data acquisition in the background and the
	 * callback is called with new samples
	 **/
	void start() {
		if (running) return;
		thr = std::thread(&FakeSensor::exec,this);
	}

	/**
	 * Stops the data acquistion
	 **/
	void stop() {
		if (!running) return;
		running = false;
		thr.join();
	}

private:
	/**
	 * Fake the arrival of data
	 **/
	void fakeEvent() {
		float value = sin(t) * 5 + 20;
		t += 0.1;
		if (nullptr != sensorCallback) {
                        sensorCallback->hasSample(value);
                }
        }

	/**
 	* Thread which runs in an endless loop.
	* Big fat warning: don't create timining
 	* with this in any real application as
	* it's not precise. Just for demo purposes.
 	*/
	void exec() {
		running = true;
		std::chrono::milliseconds duration( 100 );
		while (running) {
			std::this_thread::sleep_for( duration );
			fakeEvent();
		}
	}

private:
	SensorCallback* sensorCallback = nullptr;
	float t = 0;
	bool running = false;
	std::thread thr;
};


#endif
