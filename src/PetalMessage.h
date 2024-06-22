#pragma once
#include <Arduino.h>

const unsigned UUID_LENGTH = 16;

enum PetalMessageSource {
  DEVICE_SOURCE = 0,
  CLIENT_SOURCE = 1,
  MIDI_SOURCE   = 2,
};

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
  PetalMessageSource source;
  PetalMessageType type;
  PetalMessageAction action;
  byte * payload = nullptr;
  unsigned int payloadLength = 0;
  PetalMessage(
    PetalMessageSource s,
    PetalMessageType tp,
    PetalMessageAction act,
    const byte * pl = nullptr,
    int pll = 0) {
      source = s;
      type = tp;
      action = act;
      payloadLength = pll;
      if (pl) {
        payload = (byte *)malloc(payloadLength);
        memcpy(payload, pl, payloadLength);
      }
    }
  ~PetalMessage() { if (payload) free(payload); }
};