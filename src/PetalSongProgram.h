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
  unsigned long delay;
  unsigned long color;
  bool isStartEvent;
};

enum PetalEventType: byte {
  MIDI_EVENT = 0,
};

enum PetalProgramStatus: unsigned long {
  UNLOADED = 0,
  LOADED,
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
  NO_REQUEST,
};

class PetalSongProgram {
private:
  PetalInteroperability *interop = nullptr;
  PetalEventHandler * eventHandler;
  PetalProgramStatus processStatus;
  PetalProgramEvent * events;
  unsigned long * stopEvents;
  PetalRamp * ramps;

  unsigned int rampCount;

  byte noteCount;
  byte noteValue;

  bool processedStartEvents;
  bool processedStopEvents;
  unsigned int eventIndex;
  unsigned int eventCount;
  unsigned int stopEventsCount;
  double programStartTime;
  double minEventTime;
  PetalProgramError errorStatus;

  void initialize();
  void parseSongProgram(const byte *program, unsigned int length);
  void reset();
  void unload();
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
  PetalSongProgram(PetalInteroperability *interop, const byte *program, unsigned int length, PetalEventHandler *eventHandlerIn);
  ~PetalSongProgram();
  void process();
  void handleMessage(PetalMessage message);
  PetalProgramError getErrorStatus();
};