#include <Ticker.h>
#include "WeatherMeters.h"

Ticker ticker;
WeatherMeters meters;

void intAnemometer() {
  meters.intAnemometer();
}

void intRaingauge() {
  meters.intRaingauge();
}

void secondCount() {
  meters.secondCount();
}

void metersSetup() {
  ticker.attach(1.0, secondCount);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), intAnemometer, FALLING);
  attachInterrupt(digitalPinToInterrupt(RAIN_GAUGE_PIN), intRaingauge, FALLING);

#if defined(ENABLE_DEBUG)
  meters.init(&Serial);
#else
}

void metersLoop() {

}