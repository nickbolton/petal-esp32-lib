#pragma once
#include <Arduino.h>

enum PetalMessageType {
  REQUEST      = 0,
  RESPONSE     = 1,
  NOTIFICATION = 2,
  UNKNOWN_TYPE = 0xFF,
};

enum PetalMessageAction {
  UNLOAD            = 0,
  PROGRAM           = 1,
  PLAY              = 2,
  PAUSE             = 3,
  STOP              = 4,
  PREV_SONG_SECTION = 5,
  NEXT_SONG_SECTION = 6,
  PREV_SONG         = 7,
  NEXT_SONG         = 8,
  ADVANCE_TO_SONG   = 9,
  SEND_EVENTS       = 10,
  EVENT_FIRED       = 11,
  UNKNOWN_ACTION    = 0xFF,
};

struct PetalMessage {
  PetalMessageType type;
  PetalMessageAction action;
  byte * payload = NULL;
  unsigned int payloadLength = 0;
  PetalMessage(
    PetalMessageType tp,
    PetalMessageAction act,
    byte * pl = NULL,
    int pll = 0) : type(tp), action(act), payload(pl), payloadLength(pll) {}
};