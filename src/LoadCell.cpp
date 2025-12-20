#include "LoadCell.h"

LoadCell::LoadCell(int doutPin, int clkPin, float cf, float a)
	: DOUT(doutPin), CLK(clkPin), calibrationFactor(cf) {}

void LoadCell::reset() {
	started = false;
	lastRateUpdateTime = 0;
	cnt = 0;
	tallySum = 0.0f;
	previousWeight = 0;
	breachCnt = 0;
	emaFast = -1.0f;
	emaSlow = -1.0f;
	gapVar = 0.5f;
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
	// non-blocking tare
	scale.set_offset(avgWeight);
	// non-blocking average weight
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
	float rawRate = dw / dt;

	// EMA smoothing
	if (emaFast < 0.0f) {
		emaFast = rawRate;
	} else {
		emaFast = (1 - alphaFast) * emaFast + alphaFast * rawRate;
	}
	if (emaSlow < 0.0f) {
		emaSlow = rawRate;
	} else {
		emaSlow = (1 - alphaSlow) * emaSlow + alphaSlow * rawRate;
	}
	emaFast = fmin(emaFast, emaSlow);
	emaGap = emaSlow - emaFast;

	gapVar = (1 - betaVar) * gapVar + betaVar * (emaGap * emaGap);

	if (emaGap > stdMove * sqrt(gapVar)) {
		breachCnt++;
	} else {
		breachCnt = 0;
	}
	Serial.print("rate,");
	Serial.print(rawRate, 6); Serial.print(',');
	Serial.print(emaFast, 6); Serial.print(',');
	Serial.println(emaSlow, 6); 

	Serial.print("gap,");
	Serial.print(emaGap, 6); Serial.print(',');
	Serial.println(gapVar, 6); 

	previousWeight = netWeight;
	lastRateUpdateTime = now;
}

bool LoadCell::shouldStop() {
	if (breachCnt == conscBreach) {
		return true;
	}
	return false;
}
