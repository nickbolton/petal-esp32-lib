#pragma once
#include <Arduino.h>
#include "PetalSongProgram.h"
#include "PetalMessage.h"
#include "PetalInteroperability.h"
#include "PetalEventHandler.h"
#include <vector> 
#include <map>

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

struct MultiPartMeta {
  u_int16_t payloadLength;
  u_int8_t *payload;
};

class PetalMidiBridge {
private:

  void* _midi = nullptr;
  std::vector<PetalSongProgram *> songPrograms;
  std::map<u_int32_t, u_int32_t> songIDToIndexMap;
  PetalEventHandler *eventHandler = nullptr;
  PetalInteroperability *interop = nullptr;
  u_int8_t songIndex = 0;
  bool activateFirstSong = false;
  std::map<u_int32_t, MultiPartMeta> multiPartMeta;

  bool isConnected();
  void sendPetalRequest(PetalMessageAction action);

  ParseMessageResponse parseMessage(const byte * bytes, unsigned int length);
  ParseMessageResponse parseRawMessage(PetalMessageType type, PetalMessageAction action, const byte * bytes, unsigned int length);
  ParseMessageResponse parseMultiPartMessage(PetalMessageAction action, const byte * bytes, unsigned int length);
  PetalMessageAction parseAction(const byte * bytes);
  u_int8_t parseMultiPartNumber(const byte * bytes);
  u_int8_t parseMultiPartCount(const byte * bytes);
  u_int16_t parseMultiPartSize(const byte * bytes);
  u_int16_t parseMultiPartTotalSize(const byte * bytes);
  const byte * parseMultiPartPayload(const byte * bytes);
  PetalProgramError handleProgramMessage(PetalMessage message);
  PetalProgramError handleProgramRequest(PetalMessage message);
  PetalProgramError handleProgramActivate(PetalMessage message);
  PetalProgramError handleDebuggingEnabled(PetalMessage message);
  PetalProgramError unloadProgram();
  void activatePosition(u_int8_t position);
  void handleSongProgramRequest(PetalMessageAction action);
  void processPacket(unsigned long data);
  void sendResponse(const byte *uuid, ParseMessageResponse response);
  void handleQueueSongRequest(byte value);
  void sendAdvanceToSongMessage(byte position);
  void clearMultiPart(u_int32_t id);
  void resetMultiparts();
  MultiPartMeta startMultiPart(const byte * bytes);
  void tearDownProgram();

public:
  PetalMidiBridge(PetalInteroperability *interop);
  ~PetalMidiBridge();
  void setup();
  void process();
  void receiveSysExMessage(byte * message, unsigned length);
  void receiveControlChange(byte channel, byte number, byte value);
  unsigned short getSongIndex();
  void setSongIndex(unsigned short index);
  PetalSongProgram* currentSongProgram();
};
