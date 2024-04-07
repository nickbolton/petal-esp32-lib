#include <Arduino.h>
#include "PetalUtils.h"
#include "PetalStatus.h"
#include "PetalMidiBridge.h"
#include "PetalInteroperabilityImpl.h"

PetalInteroperabilityImpl *interop = new PetalInteroperabilityImpl;
PetalStatus petalStatus = PetalStatus();
PetalMidiBridge *bridge = new PetalMidiBridge(interop);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Startup up…");
  bridge->setup();
  interop->setup(bridge);
}

void loop() {
  bridge->process();
  interop->process();
}