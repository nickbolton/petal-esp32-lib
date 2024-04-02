#include "PetalSongProgram.h"
#include "PetalSendEvents.h"
#include "PetalPlayMessage.h"
#include "utils.h"

const int MAX_EVENTS_SIZE = 512;
const uint8_t TRIANGLE_SHAPE = 0;
const uint8_t SQUARE_SHAPE = 1;
const uint8_t SINE_SHAPE = 2;
const uint8_t EXPONENTIAL_SHAPE = 3;
const double EXPONENTIAL_STEEPNESS = 15.0;
const double PI_2 = PI/2.0;

PetalSongProgram::PetalSongProgram(const byte *program, unsigned int length) {
  initialize();
  parseSongProgram(program, length);
  PETAL_LOGI("ZZZ new PetalSongProgram(%p)", this);
}

PetalSongProgram::~PetalSongProgram() {
  free(events);
  free(stopEvents);
  free(ramps);
  eventHandler = NULL;
  PETAL_LOGI("ZZZ delete PetalSongProgram(%p)", this);
}

PetalProgramError PetalSongProgram::getErrorStatus() {
  return errorStatus;
}

void PetalSongProgram::initialize() {
  processStatus = UNLOADED;
  rampCount = 0;
  processedStartEvents = false;
  processedStopEvents = false;
  eventIndex = 0;
  eventCount = 0;
  stopEventsCount = 0;
  programStartTime = 0;
  minEventTime = 0;
}

void PetalSongProgram::process() {
  processProgramEvents();
  processRampingEvents();
}

void PetalSongProgram::reset() {
  rampCount = 0;
  eventIndex = 0;
  eventCount = 0;
  processedStartEvents = false;
  processedStopEvents = false;
}

void PetalSongProgram::unload() {
  processStatus = UNLOADED;
  reset();
  stopEventsCount = 0;
  PETAL_LOGI("Song Program UNLOADED");
}

void PetalSongProgram::handleMessage(PetalMessage message) {
  errorStatus = VALID_REQUEST;
  PetalMessageAction action = message.action;
  switch (action) {
  case PetalMessageAction::UNLOAD: {
      unload();
      break;
    }
  case PetalMessageAction::PLAY: {
      programStartTime = (double)millis();
      PetalPlayMessage playMessage = PetalPlayMessage(message);
      minEventTime = (double)playMessage.getTimestamp();
      processStatus = RUNNING;
      PETAL_LOGI("Song Program STARTED at: %ld", minEventTime);
      resetRamps();
      eventIndex = 0;
      processProgramEvents();
      break;
    }
  case PetalMessageAction::PAUSE: {
      if (processStatus != RUNNING) {
        errorStatus = NOT_RUNNING;
        return;
      }
      processStatus = PAUSED;
      resetRamps();
      PETAL_LOGI("Song Program PAUSED");
      break;
    }
  case PetalMessageAction::STOP: {
      processStatus = STOPPED;
      PETAL_LOGI("Song Program STOPPED");
      resetRamps();
      processProgramEvents();
      break;
    }
  default: {
      PETAL_LOGI("unknowned status: %ld", message.action);
      errorStatus = INVALID_ACTION;
      unload();
      break;
    }
  }
}

void PetalSongProgram::parseSongProgram(const uint8_t *program, unsigned int programLength) {
  errorStatus = VALID_REQUEST;
  if (processStatus != UNLOADED) {
    unload();
  }
  if (!program || programLength < (3 * ULONG_SIZE)) { 
    PETAL_LOGI("ZZZ no song program!");
    errorStatus = INVALID_PAYLOAD_SIZE;
    return; 
  }

  reset();

  int idx = 0;

  stopEventsCount = parseULong(program, idx);
  idx += ULONG_SIZE;
  eventCount = parseULong(program, idx);
  idx += ULONG_SIZE;
  rampCount = parseULong(program, idx);
  idx += ULONG_SIZE;

  uint32_t expectedStopEventsSize = stopEventsCount * ULONG_SIZE;
  uint32_t expectedEventsSize = eventCount * (3 * ULONG_SIZE + 2); 
  uint32_t expectedRampEventsSize = rampCount * (4 * ULONG_SIZE + 3);
  uint32_t totalExpectedSize = expectedStopEventsSize + expectedEventsSize + expectedRampEventsSize;

  if (programLength < totalExpectedSize) {
    errorStatus = INVALID_PAYLOAD_SIZE;
    return;
  }

  processStatus = LOADED;
  PETAL_LOGI("Song Program LOADED");

  uint32_t parseStartTime = millis();

  PETAL_LOGI("stopEventsCount: %d, idx: %d, programLength: %d", stopEventsCount, idx, programLength);

  events = (PetalProgramEvent *)malloc(eventCount * sizeof(PetalProgramEvent));
  stopEvents = (uint32_t *)malloc(stopEventsCount * sizeof(uint32_t));
  ramps = (PetalRamp *)malloc(rampCount * sizeof(PetalRamp));

  uint32_t eventIndex = 0;
  while (eventIndex < stopEventsCount) {
    stopEvents[eventIndex] = parseULong(program, idx);
    idx += ULONG_SIZE;
    eventIndex++;
  }

  PETAL_LOGI("eventCount: %d, idx: %d", eventCount, idx);

  eventIndex = 0;
  while (eventIndex < eventCount) {
    PetalProgramEvent event;
    event.packet = parseULong(program, idx);
    idx += ULONG_SIZE;
    event.beat = parseFloat(program, idx);
    idx += FLOAT_SIZE;
    event.delay = parseULong(program, idx);
    idx += ULONG_SIZE;
    event.color = parseULong(program, idx);
    idx += ULONG_SIZE;
    event.volumeValue = program[idx++];
    event.isStartEvent = program[idx++];

    PETAL_LOGI("event packet: %04x, isStartEvent: %d", event.packet, event.isStartEvent);
    events[eventIndex] = event;
    eventIndex++;
  }

  PETAL_LOGI("rampCount: %d, idx: %d", rampCount, idx);

  int rampIndex = 0;
  while (rampIndex < rampCount) {
    uint32_t rampSource = parseULong(program, idx);
    idx += ULONG_SIZE;

    PetalRamp ramp;
    ramp.currentValue = -1;
    ramp.source = rampSource;
    ramp.start = (double)parseULong(program, idx);
    idx += ULONG_SIZE;
    ramp.cycleStart = ramp.start;
    ramp.duration = (double)parseULong(program, idx);
    ramp.end = ramp.start + ramp.duration;
    idx += ULONG_SIZE;
    ramp.dutyCycle = (double)parseULong(program, idx);
    idx += ULONG_SIZE;
    ramp.startValue = program[idx++];
    ramp.endValue = program[idx++];
    ramp.reversed = 0;
    ramp.shape = program[idx++];
    ramp.count = 0;

    if (processStatus == RUNNING) {
      ramp.ended = minEventTime >= ramp.start;
    } else {
      ramp.ended = false;
    }

    if (ramp.duration > 0 && ramp.dutyCycle > 0) {
      ramps[rampIndex++] = ramp;
      PETAL_LOGI("ramp source: %04x", rampSource);
    }
  }
  rampCount = rampIndex;

  processProgramEvents();
}

void PetalSongProgram::processStartEvents() {
  if (processedStartEvents) {
    return;
  }
  for (int i=0; i<eventCount; i++) {
    if (events[i].isStartEvent) {
      // sendRemoteLogging(appendInt("processing start packet: ", i) + appendInt(" of ", eventCount) + appendLong(" color: ", events[i].color) + " " + packetString(events[i].packet) + "\n");
      PetalSendEvents::processPacket(events[i].packet);
      if (eventHandler) {
        (*eventHandler)(&(events[i].beat));
      }
      // setCurrentStatusColor(events[i].color);
    }
  }
  processedStartEvents = true;
}

void PetalSongProgram::setEventHandler(EventFiredCallback fptr) {
  eventHandler = fptr;
}

void PetalSongProgram::processStopEvents() {
  if (processedStopEvents) {
    return;
  }
  for (int i=0; i<stopEventsCount; i++) {
    // sendRemoteLogging(appendInt("processing stop event: ", i) + appendInt(" of ", stopEventsCount) + " : " + packetString(stopEvents[i]) + "\n");
    PetalSendEvents::processPacket(stopEvents[i]);
    if (eventHandler) {
      (*eventHandler)(&(events[i].beat));
    }
  }
  processedStopEvents = true;
}

void PetalSongProgram::processProgramEvents() {
  switch (processStatus) {
    case UNLOADED:
      break;
    case LOADED:
      processStartEvents();
      break;
    case RUNNING:
      processRunningEvents();
      break;
    case STOPPED:
      processStopEvents();
      break;
  }
}

void PetalSongProgram::processRunningEvents() {

  // PETAL_LOGI("processStatus: %d, eventIndex: %d, eventCount: %d", processStatus, eventIndex, eventCount);
  if (eventIndex >= eventCount || processStatus != RUNNING) {
    // if (isDebugLogging) {
    //   debug(appendLong("eventIndex: ", eventIndex));
    //   debug(appendLong("eventCount: ", eventCount));
    //   debug("Bailing 1…");
    // }
    return;
  }

  uint32_t now = millis();
  uint32_t elapsedTime = now - programStartTime;

  // if (isDebugLogging) {
  //   debug(appendLong("minEventTime: ", minEventTime));
  //   debug(appendLong("elapsedTime: ", elapsedTime));
  //   debug(appendInt("eventIndex: ", eventIndex));
  //   debug(appendLong("delay: ", events[eventIndex].delay));
  // }

  // first skip all events prior to minEventTime
  for (; eventIndex < eventCount; eventIndex++) {
    if (events[eventIndex].delay >= minEventTime) {
      break;
    }
    // sendRemoteLogging(appendLong("skipping event: ", eventIndex) + "\n");
  }

  for (; eventIndex < eventCount; eventIndex++) {
    if ((events[eventIndex].delay - minEventTime) > elapsedTime) {
      // debug("Bailing 3…");
      return;
    }
    // sendRemoteLogging(appendInt("processing packet: ", eventIndex) + appendInt(" of ", eventCount) + appendLong(" color: ", events[eventIndex].color) + " " + packetString(events[eventIndex].packet) + "\n");
    PetalSendEvents::processPacket(events[eventIndex].packet);
    // setCurrentStatusColor(events[eventIndex].color);
  }
}

// Ramps

void PetalSongProgram::resetRamps() {
  for (int i=0; i<rampCount; i++) {
    ramps[i].currentValue = -1;
    ramps[i].reversed = false;
    ramps[i].cycleStart = ramps[i].start;
    ramps[i].ended = minEventTime > ramps[i].start;
    ramps[i].count = 0;
    // debug(appendInt("ramp: ", i) + appendInt(" ended: ", ramps[i].ended) + "\n");
  }
}

void PetalSongProgram::performRamp(int index, double progress, double linearProgress, double elapsed, bool force) {
  PetalRamp ramp = ramps[index];
  int8_t value = ramp.startValue + (int8_t)((ramp.endValue - ramp.startValue) * progress);
  if (ramp.shape == SQUARE_SHAPE) {
    if (progress < 0.5) {
      value = ramp.startValue;
    } else {
      value = ramp.endValue;
    }
  }

  // if (isDebugLogging) {
  //   debug(appendUint8("ramp.eventType: ", ramp.eventType) + "\n");
  //   debug(appendUint8("ramp.startValue: ", ramp.startValue) + "\n");
  //   debug(appendUint8("ramp.endValue: ", ramp.endValue) + "\n");
  //   debug(appendUint8("ramp.currentValue: ", ramp.currentValue) + "\n");
  //   debug(appendDouble("progress: ", progress) + "\n");
  //   debug(appendUint8("value: ", value) + "\n");
  // }

  if (value == ramp.currentValue) {
    // if (isDebugLogging) {
    //   debug("Bailing on ramp value .. no change!\n");
    // }
    return;
  }
  ramps[index].currentValue = value;
  ramps[index].count++;

  uint32_t basePacket = ramp.source & 0xFFFFFF00;
  uint32_t packet = basePacket + (uint8_t)value;
  PetalSendEvents::processPacket(packet);
}

double PetalSongProgram::convertProgressToRampShape(int index, double progress) {
  PetalRamp ramp = ramps[index];

  if (ramp.shape == SINE_SHAPE) {
    return 0.5 + (0.5 * sin((PI * progress) - PI_2));
  }
  if (ramp.shape == EXPONENTIAL_SHAPE) {
    return (pow(EXPONENTIAL_STEEPNESS, progress) - 1) / (EXPONENTIAL_STEEPNESS - 1);
  }
  return progress;
}

void PetalSongProgram::processRampingEvents() {
  if (processStatus != RUNNING) {
    return;
  }
  double now = (double)millis();

  for (int i=0; i<rampCount; i++) {
    preProcessRamp(i, now);
  }
}

void PetalSongProgram::preProcessRamp(int index, double now) {
  PetalRamp ramp = ramps[index];
  double elapsedTime = now - programStartTime;
  double timeInSong = elapsedTime + minEventTime;
  double overallProgress = (timeInSong - ramp.start) / ramp.duration;
  double shapeOverallProgress = convertProgressToRampShape(index, overallProgress);

  // if (isDebugLogging) {
  //   debug(appendDouble("minEventTime: ", minEventTime) + appendDouble("ramp.start: ", ramp.start));
  // }

  if (ramp.ended) {
    return;
  }

  if (overallProgress >= 1.0) {
    ramps[index].ended = true;
    // sendRemoteLogging(appendInt("end of ramp: ", index) + appendDouble(" start ", ramp.start) + appendDouble(" end ", ramp.end) + appendDouble(" minEventTime: ", (double)minEventTime) + appendDouble(" overallProgress: ", overallProgress) + appendUint8(" ramp.currentValue: ", ramp.currentValue) + "\n");
    if (ramp.currentValue != ramp.endValue && ramp.duration == ramp.dutyCycle && ramp.shape != SQUARE_SHAPE) {
      performRamp(index, 1.0, overallProgress, elapsedTime, true);
      return;
    } else if (overallProgress > 1.0) {
      return;
    }
  }

  double cycleProgress = (timeInSong - ramp.cycleStart) / ramp.dutyCycle;
  if (cycleProgress > 1.0) {
    ramps[index].reversed = !ramps[index].reversed;
    ramps[index].cycleStart += ramp.dutyCycle;
    preProcessRamp(index, now);
    return;
  }

  double truncatedProgress = min(max(cycleProgress, 0.0), 1.0);
  if (ramp.reversed) {
    truncatedProgress = 1.0 - truncatedProgress;
  }
  double shapeProgress = convertProgressToRampShape(index, truncatedProgress);

  if (timeInSong >= ramp.cycleStart) {
    performRamp(index, shapeProgress, truncatedProgress, elapsedTime, false);
  }
}