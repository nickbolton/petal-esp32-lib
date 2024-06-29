#pragma once
#include <Arduino.h>
#include "PetalMessage.h"
#include "PetalEventHandler.h"

struct PetalRamp {
  unsigned long source;
  double start;
  double end;
  double duration;
  double dutyCycle;
  double cycleStart;
  byte startValue;
  byte endValue;
  byte shape;
  byte reversed;
  byte currentValue;
  bool ended;
  int count;
};

struct PetalProgramEvent {
  unsigned long packet;
  float beat;
  u_int32_t delay;
  u_int32_t color;
  bool isStartEvent;
};

enum PetalEventType: byte {
  MIDI_EVENT = 0,
};

enum PetalProgramStatus: unsigned long {
  LOADED = 0,
  RUNNING,
  PAUSED,
  STOPPED,
};

enum PetalProgramError: byte {
  VALID_REQUEST = 0,
  INVALID_PAYLOAD_HEADER,
  INVALID_PAYLOAD_SIZE,
  NOT_RUNNING,
  INVALID_ACTION,
  NO_PROGRAM,
  INVALID_MULTI_PART,
  NO_REQUEST,
};

class PetalSongProgram {
private:
  PetalInteroperability *interop = nullptr;
  PetalEventHandler * eventHandler;
  PetalProgramStatus programStatus;
  PetalProgramEvent * events;
  unsigned long * stopEvents;
  PetalRamp * ramps;

  u_int32_t rampCount;

  byte noteCount;
  byte noteValue;

  u_int32_t songId;
  bool processedStartEvents;
  bool processedStopEvents;
  u_int32_t eventIndex;
  u_int32_t eventCount;
  u_int32_t stopEventsCount;
  double programStartTime;
  double minEventTime;
  PetalProgramError errorStatus;
  bool active = false;

  void initialize();
  void parseSongProgram(const byte *program, u_int32_t length);
  void reset();
  void resetRamps();
  void processProgramEvents();
  void processStartEvents();
  void processRunningEvents();
  void processStopEvents();
  void processRampingEvents();
  void processPacket(unsigned long data);

  void performRamp(int index, double progress, double linearProgress, double elapsed, bool force);
  double convertProgressToRampShape(int index, double progress);
  void preProcessRamp(int index, double now);

public:
  PetalSongProgram(PetalInteroperability *interop, const byte *program, u_int32_t length, PetalEventHandler *eventHandlerIn);
  ~PetalSongProgram();
  void process();
  void handleMessage(PetalMessage message);
  PetalProgramError getErrorStatus();
  bool isActive();
  void setActive(bool value);
  void pause();
  void play();
  u_int32_t getSongId();
  PetalProgramStatus getProgramStatus();
};