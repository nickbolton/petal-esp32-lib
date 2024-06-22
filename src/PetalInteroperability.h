#pragma once
#include <Arduino.h>

struct PetalInteroperability {
  virtual bool isConnected() = 0;
  virtual void sendSysExMessage(const byte * message, unsigned length) = 0;
  virtual void sendProgramChange(byte channel, byte number) = 0;
  virtual void sendControlChange(byte channel, byte number, byte value) = 0;
  virtual void setCurrentColor(unsigned long color);
};
