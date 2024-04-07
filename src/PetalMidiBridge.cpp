#include "PetalMidiBridge.h"
#include "PetalUtils.h"

byte incomingMidiChannel = 16;
const byte songProgramRequestNumber = 101;

PetalMidiBridge::PetalMidiBridge(PetalInteroperability *interopIn) {
  setup();
  interop = interopIn;
  eventHandler = new PetalEventHandler(interop);
}

PetalMidiBridge::~PetalMidiBridge() {
  if (program) {
    delete program;
    program = nullptr;
  }
  if (eventHandler) {
    delete eventHandler;
  }
  eventHandler = nullptr;
}

void PetalMidiBridge::handleSongProgramRequest(PetalMessageAction action) {
  sendPetalRequest(action);
  if (!program) { return; }
  switch (action) {
    case PLAY:
      program->handleMessage(PetalMessage(REQUEST, action));
    default:
    break;
  }
}

void PetalMidiBridge::setup() {
  PETAL_LOGI("Setting up petal bridge…");
}

void PetalMidiBridge::process() {
  PETAL_LOGI("loop!!");
  if (program) {
    program->process();
  }
}

void PetalMidiBridge::receiveControlChange(byte channel, byte number, byte value) {
  if (channel != incomingMidiChannel) { return; }
  switch (number) {
    case songProgramRequestNumber:
      handleSongProgramRequest((PetalMessageAction)value);
    break;
    default:
    break;
  }
}

void PetalMidiBridge::receiveSysExMessage(byte * data, unsigned length) {
  if (!isConnected()) {
    return;
  }
  // F0 + manufacturer ID
  if (length < 4) {
    return;
  }
  if (data[0] != 0xF0 || data[1] != 0x0 || data[2] != 0x0 || data[3] != MANUFACTURER_ID || data[length-1] != 0xF7) {
    return;
  }
  PetalUtils::logSysExMessage("RX SYSEX", data, length);

  unsigned int prefixLength = 4;
  byte *messageBytes = data + prefixLength; // leave off sysex prefix
  unsigned int messageLength = length - 5; // leave off sysex suffix

  PetalUtils::logBuffer("MSG", messageBytes, messageLength);

  PetalProgramError programError = parseMessage(messageBytes, messageLength);

  // send response message
  // reuse the original message payload
  messageBytes[16] = RESPONSE;
  messageBytes[18] = programError;
  unsigned int responseLength = 0;
  PetalUtils::logBuffer("RESP", messageBytes, 19);
  PetalUtils::encode7BitEncodedPayload(messageBytes, 19, &responseLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(messageBytes, responseLength);
  }
}

bool PetalMidiBridge::isConnected() {
  if (!interop) { return false; }
  return interop->isConnected();
}
  
void PetalMidiBridge::sendPetalRequest(PetalMessageAction action) {
  unsigned int encodedLength = PetalUtils::sevenBitEncodingPayloadOffset(18);
  byte payload[encodedLength] = {
    // uuid
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    // type
    REQUEST,
    // action
    action,
  };
  unsigned int responseLength = 0;
  PetalUtils::encode7BitEncodedPayload(payload, 18, &responseLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(payload, responseLength);
  }
}

PetalProgramError PetalMidiBridge::handleProgramRequest(PetalMessage message) {
  program = new PetalSongProgram(message.payload, message.payloadLength, eventHandler);
  return program->getErrorStatus();
}

PetalProgramError PetalMidiBridge::handleProgramMessage(PetalMessage message) {
  if (program) {
    // we have an program established and need to process a message
    program->handleMessage(message);
    PetalProgramError programError = program->getErrorStatus();
    if (message.action == UNLOAD) {
      if (program) {
        delete program;
        program = NULL;
      }
    }
    return programError;
  } else {
    return NO_PROGRAM;
  }
}

PetalProgramError PetalMidiBridge::parseMessage(byte * bytes, unsigned int length) {
  if (!bytes || length < 18) { 
    PETAL_LOGD("PMB : dropping empty message…");
    return INVALID_PAYLOAD_HEADER; 
  }

  unsigned int payloadLength = 0;

  // bytes is decoded in place
  PetalUtils::logBuffer("RAW MSG", bytes, length);
  PetalUtils::decode7BitEncodedPayload(bytes, length, &payloadLength);

  PetalMessageType type = (PetalMessageType)bytes[16]; // first 16 bytes is the uuid
  PetalMessageAction action = (PetalMessageAction)bytes[17];

  if (type != REQUEST) {
    return NO_REQUEST;
  }

  // the message payload has the uuid, type, and action truncated from the start
  byte * payload = bytes + 18;
  payloadLength = length - 18;

  PetalUtils::logBuffer("MSG PAYLOAD", payload, payloadLength);

  PetalMessage message = PetalMessage(type, action, payload, payloadLength);
  switch (action) {
    case PROGRAM:
    case SEND_EVENTS:
      return handleProgramRequest(message);
    default:
      return handleProgramMessage(message);
  }
}

void PetalMidiBridge::processEvents(const byte *bytes, unsigned int length) {

  unsigned int idx = 0;

  unsigned long eventCount = PetalUtils::parseULong(bytes, idx);
  idx += ULONG_SIZE;
  unsigned long rampCount = PetalUtils::parseULong(bytes, idx);
  idx += ULONG_SIZE;

  unsigned long eventIndex = 0;
  while (eventIndex < eventCount && idx < length) {
    PetalProgramEvent event;
    event.packet = PetalUtils::parseULong(bytes, idx);
    idx += ULONG_SIZE;
    event.delay = PetalUtils::parseULong(bytes, idx);
    idx += ULONG_SIZE;
    event.color = PetalUtils::parseULong(bytes, idx);
    idx += ULONG_SIZE;
    event.volumeValue = bytes[idx++];
    event.isStartEvent = bytes[idx++];
    processPacket(event.packet);
    eventIndex++;
  }
}

void PetalMidiBridge::processPacket(unsigned long data) {
  if (!eventHandler) { return; }
  eventHandler->processPacket(data);
}