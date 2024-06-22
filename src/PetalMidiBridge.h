#pragma once
#include <Arduino.h>
#include "PetalSongProgram.h"
#include "PetalMessage.h"
#include "PetalInteroperability.h"
#include "PetalEventHandler.h"

struct ParseMessageResponse {
  PetalMessageAction action;
  PetalProgramError error;

  ParseMessageResponse(
    PetalMessageAction a,
    PetalProgramError e) {
      action = a;
      error = e;
    }
};

class PetalMidiBridge {
private:

  void* _midi = nullptr;
  PetalSongProgram *program = nullptr;
  PetalEventHandler *eventHandler = nullptr;
  PetalInteroperability *interop = nullptr;

  bool isConnected();
  void sendPetalRequest(PetalMessageAction action);

  ParseMessageResponse parseMessage(const byte * bytes, unsigned int length);
  PetalMessageAction parseAction(const byte * bytes);
  PetalProgramError handleProgramMessage(PetalMessage message);
  PetalProgramError handleProgramRequest(PetalMessage message);
  void handleSongProgramRequest(PetalMessageAction action);
  void processPacket(unsigned long data);
  void sendResponse(const byte *uuid, ParseMessageResponse response);
  void handleQueueSongRequest(byte value);
  void sendAdvanceToSongMessage(byte position);

public:
  PetalMidiBridge(PetalInteroperability *interop);
  ~PetalMidiBridge();
  void setup();
  void process();
  void processEvents(const byte *bytes, unsigned int length);
  void receiveSysExMessage(byte * message, unsigned length);
  void receiveControlChange(byte channel, byte number, byte value);
};
