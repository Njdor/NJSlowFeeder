#include "LoadCell.h"

LoadCell::LoadCell(int doutPin, int clkPin, float cf, float a)
	: DOUT(doutPin), CLK(clkPin), calibrationFactor(cf), alpha(a) {}

void LoadCell::reset() {
	weightInd = 0;
	fill(weightWindow.begin(), weightWindow.end(), 0.0f);
	weightObsCnt = 0;
	// startTime = 0;
	started = false;
	lastRateUpdateTime = 0;
	rateStoppedSince = 0;
	weightStoppedSince = 0;
	previousWeight = 0;
	smoothedRate = 0;
	minWeight = 0; maxWeight = 0;
	weightFlag = false, rateFlag = false;
}

void LoadCell::setup() {
	scale.begin(DOUT, CLK);
	scale.set_scale(calibrationFactor);
}

bool LoadCell::nonBlockingReadWeight() {
	if (cnt++ < numReadings) {
		tallySum += scale.get_units();
		return false;
	}
	else {
		avgWeight = tallySum / numReadings;
		cnt = 0;
		tallySum = 0.0f;
		return true;
	}
}

void LoadCell::start(unsigned long now) {
	if (!nonBlockingReadWeight()) { return; }
	// non blocking tare
	scale.set_offset(avgWeight);
	// non blocking average weight
	previousWeight = avgWeight - scale.get_offset();
	started = true;
	lastRateUpdateTime = millis();
	// scale.tare(numReadings);
	// previousWeight = scale.get_units(numReadings);
}

void LoadCell::update() {
	unsigned long now = millis();
	if (!started){
		start(now);
		return;
	}
	else if (now - lastRateUpdateTime < sampleInterval) { 
		return; 
	}
	
	if (!nonBlockingReadWeight()) { return; }

	// float scaleReading = scale.get_units(numReadings);
	// float scaleReading = scale.get_units();
	// float dw = abs(scaleReading - previousWeight);
	float netWeight = avgWeight - scale.get_offset();
	float dw = abs(netWeight - previousWeight);
	float dt = (now - lastRateUpdateTime) / 1000.0;

	float currRate = dw / dt;

	// EMA smoothing
	smoothedRate = alpha * currRate + (1 - alpha) * smoothedRate;

	// Rolling window to track weights
	// weightWindow[weightInd++] = scaleReading;
	weightWindow[weightInd++] = netWeight;
	weightInd = weightInd % windowSize;

	if (weightObsCnt < windowSize) weightObsCnt++;

	auto minmax = minmax_element(weightWindow.begin(), weightWindow.end());
	minWeight = *minmax.first;
	maxWeight = *minmax.second;

	for (size_t i = 0; i < weightWindow.size(); ++i) {
		Serial.print(weightWindow[i], 3); 
		if (i < weightWindow.size() - 1) Serial.print(" | ");
	}
	Serial.println();
	Serial.print("Weight diff: ");
	Serial.println(maxWeight - minWeight, 3);
	Serial.print("Smoothed Rate: "); 
	Serial.println(smoothedRate, 3); 
	Serial.println();

	// previousWeight = scaleReading;
	previousWeight = netWeight;
	lastRateUpdateTime = now;
}


bool LoadCell::shouldStop() {
    if (weightObsCnt < windowSize) {
        return false;
    }

    unsigned long now = millis();

    // --- Weight check ---
    bool weightCond = (maxWeight - minWeight < weightChangeThreshold);
    if (weightCond) {
        if (weightStoppedSince == 0) weightStoppedSince = now;
    } 
	else {
        weightStoppedSince = 0;
    }

    // --- Rate check ---
    bool rateCond = (smoothedRate < feedRateThreshold);
    if (rateCond) {
        if (rateStoppedSince == 0) rateStoppedSince = now;
    } 
	else {
        rateStoppedSince = 0;
    }

    // Flags for persistent condition 
    bool weightStuck = weightCond && (now - weightStoppedSince >= stopHoldTime);
    bool rateStuck   = rateCond   && (now - rateStoppedSince >= stopHoldTime);

    // Final stop condition
	if (weightCond) { Serial.println("Weight condition met"); }
	if (rateCond) { Serial.println("Rate condition met"); }
	if (weightStuck) { Serial.println("Weight condition stuck"); }
	if (rateStuck) { Serial.println("Rate condition stuck"); }

	return (weightCond && rateCond) || weightStuck || rateStuck;
}