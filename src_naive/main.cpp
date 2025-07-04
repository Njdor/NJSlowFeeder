#include <Arduino.h>
#include <Button.h> //uh, buttons
#include "driver/rtc_io.h" //deepsleep
#include "HX711.h" //loadcell

//deepsleep/wake defines, using EXT0 cause internal pullup power domain is needed.
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_5     // Only RTC IO are allowed

// //GPIOs for R&D Supermini, don't use.
// const int IN1MotorPin = 13; //change to 12 if motor runs in reverse
// const int IN2MotorPin = 12; //change to 13 if motor runs in reverse
// const int buttonUPpin = 5;
// const int buttonDOWNpin = 4; 
// const int HX_DOUT = 10; //hx711
// const int HX_CLK = 11; //hx711
// #define HX711CLK GPIO_NUM_11 //define HX_CLK this way as well for deepsleep pullup (hx711 in sleep)


//GPIOs (note these don't match up with the silkscreen digital callouts on the XIAO board, see schematic)
const int IN1MotorPin = 44; //change to 44 if motor runs in reverse
const int IN2MotorPin = 7; //change to 7 if motor runs in reverse
const int buttonUPpin = 5; //labeled as D4 on the silkscreen for xiao
const int buttonDOWNpin = 6; //labeled as D5 on the slikscreen for xiao
const int HX_DOUT = 9; //hx711, labeled as D10 on the silkscreen for xiao
const int HX_CLK = 8; //hx711, labeled as D9 on the silkscreen for xiao
#define HX711CLK GPIO_NUM_8 //define HX_CLK this way as well for deepsleep pullup (hx711 in sleep)

//motor variables
float MotorVoltage = 0;  //Current value of Motor Drive Voltage
  //user config
int MotorPWMFrequency = 20000; //motor PWM frequency, 20000 you can't hear, 1000 has more granular range.
float MotorStartVoltage = 2.5; //voltage the system will default to when clicking or holding up. Use 1.5V for 1000Hz analog freq, 2.7 for 20000Hz.
float BusVoltage = 3.3; //PWM Logic Level
float MotorVoltageStep = 0.1; //voltage the motor will step up per MotorUpdateTime interval when a button is held  Use 0.2 for 1000Hz, 0.1 for 20000Hz
int MotorUpdateTime = 500; //milliseconds, controls how frequently the motor voltage gets updated

int setMotorVoltage(int, float, float);
void TareScales(void);
void MakeNoise(int, int);

//Loop/Sleep Variables
u_long PrevUpdateTime = 0; 
u_long SleepTimeoutTracker = 0;
  //user config
int SleepTimeoutTime = 60000; //milliseconds, controls how long before device deep sleep when there's no weight, button presses, etc.
int SleepErrorTimeoutTime = 300000;  //sleep regardless of weight after 5 minutes of no button pushes. Used to cover for any weird scale behavior.
float WeightTimeoutMinimumBound = 2.6; //minimum weight to stay awake
float WeightTimeoutMaxiumumBound = 30.0; //maximum bean weight expected in slowfeeder, keeps awake.

//AutoDose
// float DoseStartingWeight = 0; //will be set by program later
// float DoseTarget = 20; //grams

//MotorAutoOff
u_long MotorRunTime = 0;
int MotorRunTimeout = 60000; //milliseconds to run motor
float LastWeightReading = 0;
u_long LastWeightChangeTime = 0;
const float WeightStabilityThreshold = 2;  // grams, allow small drift
const u_long WeightUnchangedTimeout = 5000;  // 5 seconds

//LastButtonPress
u_long LastButtonPress = 0;
int ButtonRunTimeout = 5000; //milliseconds to ignore motor shutoff conditions due to button press


//HX711 Scale
HX711 scale;
// float calibration_factor = -2650; //put in your calibration factor from calibration code
float calibration_factor = 750;
float scale_reading = 0;

//Button configs
Button buttonUP = Button(buttonUPpin, BUTTON_PULLDOWN, true, 50);
Button buttonDOWN = Button(buttonDOWNpin, BUTTON_PULLDOWN, true, 50);
//Button Callbacks
// void onPress(Button& b){
// 	Serial.print("onPress: ");
// 	Serial.println(b.pin);
// 	//digitalWrite(13,HIGH);
// }

// void onClick(Button& b){
// 	Serial.print("onClick: ");
// 	Serial.println(b.pin);
// 	//digitalWrite(13,HIGH);
// }

void onRelease(Button& b){
	Serial.print("onRelease: ");
	Serial.println(b.pin);
	if(b.buttonstatus == 0){
    Serial.print("Click: ");
    Serial.println(b.pin);
    b.buttonstatus = 2;
  }
  else if(b.buttonstatus == 1){
    Serial.print("HoldRelease: ");
    Serial.println(b.pin);
    b.buttonstatus = 0;
  }
  
  else {
    Serial.print("Unhandled pin triggered - release");
    Serial.println(b.buttonstatus);
  }
}

void onHold(Button& b){
	Serial.print("Held: ");
	Serial.println(b.pin);
  b.buttonstatus = 1;
  
}

//deepsleep related
/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void setup() {
  //motor configs
  pinMode(IN1MotorPin, OUTPUT);
  analogWriteFrequency(MotorPWMFrequency); //20000 is out of hearing range, 1000 seems to be fine with the motor controller in the housing.
  pinMode(IN2MotorPin, OUTPUT);
  analogWrite(IN2MotorPin, 0);

  //start serial for ButtonDebug
  Serial.begin(115200);
  Serial.println("Serial Start");

  print_wakeup_reason();

  //start scale
  scale.begin(HX_DOUT, HX_CLK);
  scale.set_scale(calibration_factor);
  scale.tare(); //assume no beans present in slow feeder when turned on. Alternatively, can experiement with zero factor below.

  // Assign callback functions
  //buttonUP.pressHandler(onPress);
  //buttonUP.clickHandler(onClick);
  buttonUP.releaseHandler(onRelease);
  buttonUP.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger

  //buttonDOWN.pressHandler(onPress);
  //buttonDOWN.clickHandler(onClick);
  buttonDOWN.releaseHandler(onRelease);
  buttonDOWN.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger
  
  
  //Methods to prevent scale taring before button release. Might be worth pairing with zero factor to create robust detection of unit resting on a counter.
  Serial.println(digitalRead(buttonUPpin));
  Serial.println(digitalRead(buttonDOWNpin));
  while(digitalRead(buttonUPpin) != LOW || digitalRead(buttonDOWNpin) != LOW){ 
    delay(500);
    Serial.println("Button pressed, delay startup tare");
  } 
  //delay for 2 second to allow for time to set the slowfeeder on the counter.
  // Serial.println("Start Delay startup");
  // delay(2000);
  // Serial.println("End Delay Startup");
  // scale.tare();
  // //spin motor to indicate completion of startup
  // analogWriteFrequency(1000);
  // setMotorVoltage(IN1MotorPin, BusVoltage, 0.5);
  // delay(500);
  // setMotorVoltage(IN1MotorPin, BusVoltage, 0);
  // analogWriteFrequency(MotorPWMFrequency);
  TareScales();
  //reset status, button lib has startup bugs - will always trigger a release and click mode per button when process is called the first time. Also cover button pressed reset from scale method
  buttonUP.process();
  buttonDOWN.process();
  buttonUP.buttonstatus = 0;
  buttonDOWN.buttonstatus = 0;
  Serial.println(buttonUP.buttonstatus);
  Serial.println(buttonDOWN.buttonstatus);

  //set time trackers to current time
  PrevUpdateTime = millis();
  SleepTimeoutTracker = PrevUpdateTime;
}

void loop() {
  int updatetimebool = ((millis() - PrevUpdateTime) > MotorUpdateTime);
  if(((millis() - SleepTimeoutTracker) > SleepTimeoutTime) || ((millis() - LastButtonPress) > SleepErrorTimeoutTime)){
      //put HX711 to sleep
      MakeNoise(1750, 400);
      MakeNoise(1500, 400);
      MakeNoise(1000, 400);
      rtc_gpio_pulldown_dis(HX711CLK);
      rtc_gpio_pullup_en(HX711CLK);
      //begin going to sleep :)
      esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);  //1 = High, 0 = Low
      // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level during deepsleep.
      rtc_gpio_pullup_dis(WAKEUP_GPIO);
      rtc_gpio_pulldown_en(WAKEUP_GPIO);
      Serial.println("Going to sleep now");
      esp_deep_sleep_start();
      Serial.println("If you see this, no you didn't");
  }
  if(updatetimebool){
    //read scale every motor update interval
    scale_reading = scale.get_units();
    Serial.println(scale_reading, 1);
  }
  //check button states
  buttonUP.process();
  buttonDOWN.process();
  
  //time state evaluations
  if(buttonUP.buttonstatus != 0 || buttonDOWN.buttonstatus != 0){ LastButtonPress = millis();} //track last button active time
  if(buttonUP.buttonstatus != 0 || buttonDOWN.buttonstatus != 0 || (scale_reading > WeightTimeoutMinimumBound && scale_reading < WeightTimeoutMaxiumumBound)){ SleepTimeoutTracker = millis();} //delay sleep
  
  //button status evaluations
  if(buttonUP.buttonstatus == 1){
    //do hold things
    //Serial.println("holdUP");
    if(updatetimebool){
      if(MotorVoltage == 0){MotorVoltage = MotorStartVoltage;}
      MotorVoltage = constrain(MotorVoltage + MotorVoltageStep, 0, BusVoltage);
      Serial.print("MotorVoltage: ");
      Serial.println(MotorVoltage, 2);
      setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage);
    }
  }
  else if(buttonUP.buttonstatus == 2){
    //do click things
    //Serial.println("clickUP");
    setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage = MotorStartVoltage);
    // DoseStartingWeight = scale.get_units(3);
    //reset buttonUP state
    buttonUP.buttonstatus = 0;
  }

  if(buttonDOWN.buttonstatus == 1){
    //do hold things
    //Serial.println("holdDOWN");
    if(updatetimebool && MotorVoltage > 0){
      MotorVoltage = constrain(MotorVoltage - MotorVoltageStep, 0, BusVoltage);
      Serial.print("MotorVoltage: ");
      Serial.println(MotorVoltage, 2);
      setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage);
    }
    else if(MotorVoltage == 0){
      TareScales();
    }
  }
  else if(buttonDOWN.buttonstatus == 2){
    //do click things
    //Serial.print("clickDOWN");
    setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage = 0);
    //reset buttonUP state
    buttonDOWN.buttonstatus = 0;
  }
  
  if (MotorVoltage > 0) {
    if (MotorRunTime == 0) {
      MotorRunTime = millis();
      LastWeightReading = scale_reading;
      LastWeightChangeTime = millis();
    }
    // Check for weight stability
    if (abs(scale_reading - LastWeightReading) > WeightStabilityThreshold) {
      // weight changed → reset timer
      LastWeightReading = scale_reading;
      LastWeightChangeTime = millis();
    } else if (millis() - LastWeightChangeTime > WeightUnchangedTimeout) {
      // weight hasn't changed in 5s → stop motor
      setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage = 0);
      Serial.println("Motor auto-stopped: weight unchanged for 5s");
      MotorRunTime = 0;
    }

    // Existing 30s timeout logic
    else if (millis() - MotorRunTime > MotorRunTimeout) {
      setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage = 0);
      char buffer[64];
      sprintf(buffer, "Motor auto-off after %.1f seconds", MotorRunTimeout / 1000.0);
      Serial.println(buffer);
      MotorRunTime = 0;
    }
  } else {
    MotorRunTime = 0;
  }
  // // Motor auto-off after user defined seconds
  // if (MotorVoltage > 0) {

  //   if (MotorRunTime == 0) {
  //     MotorRunTime = millis();  // Start tracking
  //   } 
  //   else if (millis() - MotorRunTime > MotorRunTimeout) {  // 30 sec timeout
  //     setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage = 0);
  //     char buffer[64];
  //     sprintf(buffer, "Motor auto-off after %.1f seconds", MotorRunTimeout / 1000.0);
  //     Serial.println(buffer);
  //     MotorRunTime = 0;  // Reset timer
  //   }
  // } 
  // else {
  //   MotorRunTime = 0;
  // }
  // if(DoseStartingWeight > 0){
  //   if(scale_reading < 0.75){
  //     setMotorVoltage(IN1MotorPin, BusVoltage, MotorVoltage = 0);
  //     DoseStartingWeight = 0;
  //   }
  // }
  // if(updatetimebool){
  //   PrevUpdateTime = millis();
  // }
  delay(10);
}

int setMotorVoltage(int driverPin, float BusVoltage, float MotorVoltage) {
  if(MotorVoltage > BusVoltage){return 1;}
  else{
    int setpoint = (int)((MotorVoltage / BusVoltage) * 255);
    analogWrite(driverPin, setpoint);
    return 0;
  }
}

void TareScales(void){
  Serial.println("Delay tare");
  MakeNoise(1000, 500);
  delay(2000);
  Serial.println("End Delay tare");
  scale.tare(10);
  //spin motor to indicate completion of tare
  MakeNoise(1000, 500);
  MakeNoise(1500, 500);
}

void MakeNoise(int Frequency, int duration) {
  Serial.println("Making Noise");
  analogWriteFrequency(Frequency);
  setMotorVoltage(IN1MotorPin, BusVoltage, 0.3);
  delay(duration);
  setMotorVoltage(IN1MotorPin, BusVoltage, 0);
  analogWriteFrequency(MotorPWMFrequency);
}