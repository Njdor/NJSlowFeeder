#include "Buzzer.h"

Buzzer::Buzzer(int pin, int channel)
    : buzzerPin(pin), buzzerChannel(channel) {}

void Buzzer::setup() {
    rtc_gpio_hold_dis((gpio_num_t)buzzerPin);  
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    ledcAttachPin(buzzerPin, buzzerChannel);
    ledcSetup(buzzerChannel, 2000, 8); // 2kHz default, 8-bit resolution
}

void Buzzer::makeSound(int frequency, int duration) {
    ledcWriteTone(buzzerChannel, frequency); // Play tone
    delay(duration);                         // Wait
    ledcWriteTone(buzzerChannel, 0);         // Stop tone
}