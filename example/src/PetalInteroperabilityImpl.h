#pragma once

#include "PetalInteroperability.h"
#include "PetalMidiBridge.h"

class PetalInteroperabilityImpl : public PetalInteroperability {
protected:
bool _isConnected = false;
private:
PetalMidiBridge *_bridge = nullptr;

public:
  PetalInteroperabilityImpl();
  ~PetalInteroperabilityImpl();

  bool isConnected();
  void sendSysExMessage(const byte * message, unsigned length);
  void sendProgramChange(byte channel, byte number);
  void sendControlChange(byte channel, byte number, byte value);

  PetalMidiBridge *bridge() { return _bridge; };
  void setup(PetalMidiBridge *bridge);
  void process();
  void setConnected(bool b);
  void processPacket(uint32_t data);
 };
