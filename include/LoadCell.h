#ifndef LOADCELL_H
#define LOADCELL_H

#include <Arduino.h>
#include "HX711.h"
#include <array>
#include <cmath>

using namespace std;

class LoadCell {
public:
	LoadCell(int doutPin, int clkPin, float calibrationFactor, float alpha = 0.8);

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

	// Timing
	unsigned long lastRateUpdateTime = 0;
	const unsigned long sampleInterval = 500; // controls how frequent (ms) loadCell update readings

	// Params used to stop
	const float alphaFast = 0.4f;
	const float alphaSlow = 0.05f;
	const float betaVar = 0.3;
	const float stdMove = 1.3f;
	const int conscBreach = 2;
	int breachCnt = 0;
	float emaFast = -1.0f;
	float emaSlow = -1.0f;
	float emaGap;
	float gapVar = 0.5f;
	float s = 0.0f;

	// Params to read weights
	const int numReadings = 10;
	int cnt = 0;
	float tallySum = 0.0f;
	float avgWeight = 0.0f;

	void start(unsigned long now);
	double normal_cdf(double x);
	// bool started() const;
};

#endif