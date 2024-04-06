#pragma once
#include <Arduino.h>
#include "PetalInteroperability.h"

class PetalEventHandler {
private:

  PetalInteroperability *interop = nullptr;

public:
  PetalEventHandler(PetalInteroperability *interop);
  ~PetalEventHandler();
  void sendSysExMessage(const byte *payload, unsigned payloadLength);
  void sendProgramChange(byte channel, byte number);
  void sendControlChange(byte channel, byte number, byte value);
  void processPacket(uint32_t data);
  void onEventFired(float * beat);
};