#include "PetalSongProgram.h"
#include "PetalPlayMessage.h"
#include "PetalUtils.h"

const int MAX_EVENTS_SIZE = 512;
const byte TRIANGLE_SHAPE = 0;
const byte SQUARE_SHAPE = 1;
const byte SINE_SHAPE = 2;
const byte EXPONENTIAL_SHAPE = 3;
const double EXPONENTIAL_STEEPNESS = 15.0;
const double PI_2 = PI/2.0;

PetalSongProgram::PetalSongProgram(PetalInteroperability *iop, const byte *program, unsigned int length, PetalEventHandler *eventHandlerIn) {
  interop = iop;
  eventHandler = eventHandlerIn;
  initialize();
  parseSongProgram(program, length);
}

PetalSongProgram::~PetalSongProgram() {
  free(events);
  free(stopEvents);
  free(ramps);
  eventHandler = nullptr;
  eventHandler = nullptr;
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

void PetalSongProgram::parseSongProgram(const byte *program, unsigned int programLength) {
  errorStatus = VALID_REQUEST;
  if (processStatus != UNLOADED) {
    unload();
  }
  PetalUtils::logBuffer("PROGRAM", program, programLength);
  if (!program || programLength < (3 * ULONG_SIZE)) { 
    PETAL_LOGI("ZZZ no song program!");
    errorStatus = INVALID_PAYLOAD_SIZE;
    return; 
  }

  reset();

  int idx = 0;

  stopEventsCount = PetalUtils::parseULong(program, idx);
  idx += ULONG_SIZE;
  eventCount = PetalUtils::parseULong(program, idx);
  idx += ULONG_SIZE;
  rampCount = PetalUtils::parseULong(program, idx);
  idx += ULONG_SIZE;

  noteCount = program[idx++];
  noteValue = program[idx++];

  unsigned long expectedStopEventsSize = stopEventsCount * ULONG_SIZE;
  unsigned long expectedEventsSize = eventCount * (3 * ULONG_SIZE + 2); 
  unsigned long expectedRampEventsSize = rampCount * (4 * ULONG_SIZE + 3);
  unsigned long totalExpectedSize = expectedStopEventsSize + expectedEventsSize + expectedRampEventsSize;

  PETAL_LOGI("stopEventsCount: %u, eventCount: %u, rampCount: %u", stopEventsCount, eventCount, rampCount);
  PETAL_LOGI("programLength: %u, totalExpectedSize: %u", programLength, totalExpectedSize);

  if (programLength < totalExpectedSize) {
    errorStatus = INVALID_PAYLOAD_SIZE;
    return;
  }

  processStatus = LOADED;
  PETAL_LOGI("Song Program LOADED");

  PETAL_LOGI("stopEventsCount: %d, idx: %d, programLength: %d", stopEventsCount, idx, programLength);

  events = (PetalProgramEvent *)malloc(eventCount * sizeof(PetalProgramEvent));
  stopEvents = (unsigned long *)malloc(stopEventsCount * sizeof(unsigned long));
  ramps = (PetalRamp *)malloc(rampCount * sizeof(PetalRamp));

  unsigned long eventIndex = 0;
  while (eventIndex < stopEventsCount) {
    stopEvents[eventIndex] = PetalUtils::parseULong(program, idx);
    idx += ULONG_SIZE;
    eventIndex++;
  }

  PETAL_LOGI("eventCount: %d, idx: %d", eventCount, idx);

  eventIndex = 0;
  while (eventIndex < eventCount) {
    PetalProgramEvent event;
    event.packet = PetalUtils::parseULong(program, idx);
    idx += ULONG_SIZE;
    event.beat = PetalUtils::parseFloat(program, idx);
    idx += FLOAT_SIZE;
    event.delay = PetalUtils::parseULong(program, idx);
    idx += ULONG_SIZE;
    event.color = PetalUtils::parseULong(program, idx);
    idx += ULONG_SIZE;
    event.isStartEvent = program[idx++] != 0;

    char beatStr[1024];
    sprintf(beatStr, "%f", event.beat);

    PETAL_LOGI("event packet: 0x%x, beat: %s, delay: %u, color: %u, isStartEvent: %d", event.packet, beatStr, event.delay, event.color, event.isStartEvent);
    events[eventIndex] = event;
    eventIndex++;
  }

  PETAL_LOGI("rampCount: %d, idx: %d", rampCount, idx);

  unsigned int rampIndex = 0;
  while (rampIndex < rampCount) {
    unsigned long rampSource = PetalUtils::parseULong(program, idx);
    idx += ULONG_SIZE;

    PetalRamp ramp;
    ramp.currentValue = -1;
    ramp.source = rampSource;
    ramp.start = (double)PetalUtils::parseULong(program, idx);
    idx += ULONG_SIZE;
    ramp.cycleStart = ramp.start;
    ramp.duration = (double)PetalUtils::parseULong(program, idx);
    ramp.end = ramp.start + ramp.duration;
    idx += ULONG_SIZE;
    ramp.dutyCycle = (double)PetalUtils::parseULong(program, idx);
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
      PETAL_LOGI("ramp source: 0x%x", rampSource);
    }
  }
  rampCount = rampIndex;

  processProgramEvents();
}

void PetalSongProgram::processStartEvents() {
  if (processedStartEvents) {
    return;
  }
  PETAL_LOGI("processing start events…");
  for (unsigned int i=0; i<eventCount; i++) {
    if (events[i].isStartEvent) {
      // sendRemoteLogging(appendInt("processing start packet: ", i) + appendInt(" of ", eventCount) + appendLong(" color: ", events[i].color) + " " + packetString(events[i].packet) + "\n");
      PETAL_LOGI("processPacket1 event index: %d", i);
      processPacket(events[i].packet);
      if (interop) {
        interop->setCurrentColor(events[i].color);
      }
      if (eventHandler) {
        eventHandler->onEventFired(events[i].beat, noteCount, noteValue);
      }
    }
  }
  processedStartEvents = true;
  PETAL_LOGI("finished processing start events…");
}

void PetalSongProgram::processStopEvents() {
  if (processedStopEvents) {
    return;
  }
  for (unsigned int i=0; i<stopEventsCount; i++) {
    // sendRemoteLogging(appendInt("processing stop event: ", i) + appendInt(" of ", stopEventsCount) + " : " + packetString(stopEvents[i]) + "\n");
    PETAL_LOGI("processPacket2");
    processPacket(stopEvents[i]);
    if (eventHandler) {
      eventHandler->onEventFired(events[i].beat, noteCount, noteValue);
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
    case PAUSED:
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

  unsigned long now = millis();
  unsigned long elapsedTime = now - programStartTime;

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
    PETAL_LOGI("eventHandler: %ld", eventHandler);
    if (eventHandler) {
      eventHandler->onEventFired(events[eventIndex].beat, noteCount, noteValue);
    }
    processPacket(events[eventIndex].packet);
    if (interop) {
      interop->setCurrentColor(events[eventIndex].color);
    }
  }
}

// Ramps

void PetalSongProgram::resetRamps() {
  for (unsigned int i=0; i<rampCount; i++) {
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
  byte value = ramp.startValue + (byte)((ramp.endValue - ramp.startValue) * progress);
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

  unsigned long basePacket = ramp.source & 0xFFFFFF00;
  unsigned long packet = basePacket + value;
  PETAL_LOGI("processPacket3");
  processPacket(packet);
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

  for (unsigned int i=0; i<rampCount; i++) {
    preProcessRamp(i, now);
  }
}

void PetalSongProgram::preProcessRamp(int index, double now) {
  PetalRamp ramp = ramps[index];
  double elapsedTime = now - programStartTime;
  double timeInSong = elapsedTime + minEventTime;
  double overallProgress = (timeInSong - ramp.start) / ramp.duration;

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

  double truncatedProgress = std::min(std::max(cycleProgress, 0.0), 1.0);
  if (ramp.reversed) {
    truncatedProgress = 1.0 - truncatedProgress;
  }
  double shapeProgress = convertProgressToRampShape(index, truncatedProgress);

  if (timeInSong >= ramp.cycleStart) {
    performRamp(index, shapeProgress, truncatedProgress, elapsedTime, false);
  }
}

void PetalSongProgram::processPacket(unsigned long data) {
  if (!eventHandler) { return; }
  eventHandler->processPacket(data);
}