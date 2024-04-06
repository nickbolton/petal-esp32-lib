#pragma once
#include <Arduino.h>
#include "PetalMessage.h"
#include "PetalEventHandler.h"

struct PetalRamp {
  uint32_t source;
  double start;
  double end;
  double duration;
  double dutyCycle;
  double cycleStart;
  uint8_t startValue;
  uint8_t endValue;
  uint8_t shape;
  uint8_t reversed;
  int8_t currentValue;
  bool ended;
  int count;
};

struct PetalProgramEvent {
  uint32_t packet;
  float beat;
  uint32_t delay;
  uint32_t color;
  uint8_t volumeValue;
  uint8_t isStartEvent;
};

enum PetalEventType: uint8_t {
  MIDI_EVENT = 0,
};

enum PetalProgramStatus: uint32_t {
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
  PetalEventHandler * eventHandler;
  PetalProgramStatus processStatus;
  PetalProgramEvent * events;
  uint32_t * stopEvents;
  PetalRamp * ramps;

  int rampCount;

  bool processedStartEvents;
  bool processedStopEvents;
  int eventIndex;
  int eventCount;
  int stopEventsCount;
  double programStartTime;
  double minEventTime;
  PetalProgramError errorStatus;

  void initialize();
  void parseSongProgram(const uint8_t *program, unsigned int length);
  void reset();
  void unload();
  void resetRamps();
  void processProgramEvents();
  void processStartEvents();
  void processRunningEvents();
  void processStopEvents();
  void processRampingEvents();
  void processPacket(uint32_t data);

  void performRamp(int index, double progress, double linearProgress, double elapsed, bool force);
  double convertProgressToRampShape(int index, double progress);
  void preProcessRamp(int index, double now);

public:
  PetalSongProgram(const byte *program, unsigned int length, PetalEventHandler *eventHandlerIn);
  ~PetalSongProgram();
  void process();
  void handleMessage(PetalMessage message);
  PetalProgramError getErrorStatus();
};