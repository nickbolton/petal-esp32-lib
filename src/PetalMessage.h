#pragma once
#include <Arduino.h>

const unsigned UUID_LENGTH = 16;

enum PetalMessageSource {
  DEVICE_SOURCE = 0,
  CLIENT_SOURCE,
  MIDI_SOURCE,
};

enum PetalMessageType {
  REQUEST = 0,
  RESPONSE,
  NOTIFICATION,
  UNKNOWN_TYPE = 0xFF,
};

enum PetalMessageAction {
  UNLOAD = 0,
  PROGRAM,
  ACTIVATE,
  PLAY,
  PAUSE,            
  STOP,             
  PREV_SONG_SECTION,
  NEXT_SONG_SECTION,
  PREV_SONG,        
  NEXT_SONG,        
  ADVANCE_TO_SONG,  
  SEND_EVENTS,      
  EVENT_FIRED,      
  SONG_ENDED,

  DEBUGGING         = 0x7E,
  MULTI_PART        = 0x7F,
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