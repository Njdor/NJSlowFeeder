#ifndef LOADCELL_H
#define LOADCELL_H

#include <Arduino.h>
#include "HX711.h"
#include <array>

using namespace std;

class LoadCell {
public:
	LoadCell(int doutPin, int clkPin, float calibrationFactor, float alpha = 0.3);

	void setup();
	void update();               
	void reset();
	bool shouldStop();        
	bool nonBlockingReadWeight();

private:
	HX711 scale;
	float calibrationFactor;

	int DOUT, CLK;

	// track if load cell started
	bool started = false;

	// Weight tracking
	float previousWeight = 0;

	// Feed rate
	float currRate = 0;
	float smoothedRate = 0;
	float alpha;

	// Timing
	unsigned long lastRateUpdateTime = 0;
	const unsigned long sampleInterval = 500; // controls how frequent (ms) loadCell update readings

	// Feed stop detection
	unsigned long stoppedSince = 0;
	const unsigned long stopHoldTime = 2000;
	unsigned long rateStoppedSince;
	unsigned long weightStoppedSince;

	// Stopping thresholds
	const float weightChangeThreshold = 1.5;
	const float feedRateThreshold = 0.5;

	// Params used to stop
	float minWeight;
	float maxWeight;
	bool weightFlag = false;
	bool rateFlag = false;
	static const int windowSize = 5;
	int weightInd = 0;
	array<float, windowSize> weightWindow;
	int weightObsCnt = 0;

	int numReadings = 10;
	int cnt = 0;
	float tallySum = 0.0f;
	float avgWeight = 0.0f;
	void start(unsigned long now);
	// bool started() const;
};

#endif