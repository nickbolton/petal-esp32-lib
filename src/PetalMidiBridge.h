#pragma once
#include <Arduino.h>
#include "PetalSongProgram.h"
#include "PetalMessage.h"

class PetalMidiBridge {
private:

  void* _midi;
  PetalSongProgram *program;
  void (*sendSysExMessageHandler)(byte * message, unsigned length);
  bool (*isConnectedHandler)();

  void setUp();
  bool isConnected();
  static void logSysExMessageFull(String label, const byte* data, unsigned length);
  static void logSysExMessageSummary(String label, const byte* data, unsigned length);
  static void logSysExMessage(String label, const byte* data, unsigned length);
  static void onEventFired(float * beat);
  static void onSongProgramRequestHandler(PetalMessageAction action);
  static void sendPetalRequest(PetalMessageAction action);

  PetalProgramError parseMessage(byte * bytes, unsigned int length);
  PetalProgramError handleEvents(PetalMessage message);
  PetalProgramError handleProgramMessage(PetalMessage message);
  PetalProgramError handleProgramRequest(PetalMessage message);
  void sendSysExMessage(const byte *payload, unsigned payloadLength);

public:
  PetalMidiBridge();
  ~PetalMidiBridge();
  void process();
  void receiveSysExMessage(byte * message, unsigned length);
  void setSendSysExMessageHandler(void (*fptr)(byte * message, unsigned length));
  void setIsConnectedHandler(bool (*fptr)());
};
