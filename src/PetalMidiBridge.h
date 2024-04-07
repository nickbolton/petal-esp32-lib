#pragma once
#include <Arduino.h>
#include "PetalSongProgram.h"
#include "PetalMessage.h"
#include "PetalInteroperability.h"
#include "PetalEventHandler.h"

class PetalMidiBridge {
private:

  void* _midi = nullptr;
  PetalSongProgram *program = nullptr;
  PetalEventHandler *eventHandler = nullptr;
  PetalInteroperability *interop = nullptr;

  bool isConnected();
  void sendPetalRequest(PetalMessageAction action);

  PetalProgramError parseMessage(byte * bytes, unsigned int length);
  PetalProgramError handleEvents(PetalMessage message);
  PetalProgramError handleProgramMessage(PetalMessage message);
  PetalProgramError handleProgramRequest(PetalMessage message);
  void handleSongProgramRequest(PetalMessageAction action);
  void processPacket(unsigned long data);

public:
  PetalMidiBridge(PetalInteroperability *interop);
  ~PetalMidiBridge();
  void setup();
  void process();
  void processEvents(const byte *bytes, unsigned int length);
  void receiveSysExMessage(byte * message, unsigned length);
  void receiveControlChange(byte channel, byte number, byte value);
};
