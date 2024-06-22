#include "PetalMidiBridge.h"
#include "PetalUtils.h"

byte incomingMidiChannel = 16;
const byte songProgramRequestNumber = 101;
const byte queueSongRequestNumber = 102;

long lastMessageTime = 0;

PetalMidiBridge::PetalMidiBridge(PetalInteroperability *interopIn) {
  interop = interopIn;
  eventHandler = new PetalEventHandler(interop);
}

PetalMidiBridge::~PetalMidiBridge() {
  if (program) {
    delete program;
    PETAL_LOGI("ZZZ nulling program1");
    program = nullptr;
  }
  if (eventHandler) {
    delete eventHandler;
  }
  eventHandler = nullptr;
}

void PetalMidiBridge::handleSongProgramRequest(PetalMessageAction action) {
  PETAL_LOGI("handleSongProgramRequest program %x, action: %d", program, action);
  if (!program) { return; }
  sendPetalRequest(action);
  switch (action) {
    case PLAY:
    case STOP:
      program->handleMessage(PetalMessage(MIDI_SOURCE, REQUEST, action));
    default:
    break;
  }
}

void PetalMidiBridge::setup() {
  PETAL_LOGI("Setting up petal bridge…");
}

void PetalMidiBridge::process() {
  if (program) {
    program->process();
  }
  long now = millis();
  // if (now - lastMessageTime >= 2000) {
  //   byte bytes[] = {1,2,3,4,5};
  //   sendResponse(bytes, VALID_REQUEST);
  //   lastMessageTime = now;
  // }
}

void PetalMidiBridge::receiveControlChange(byte channel, byte number, byte value) {
  PETAL_LOGI("receiveControlChange on channel %d, number %d, value %d", channel, number, value);
  if (channel != incomingMidiChannel) { return; }
  switch (number) {
    case songProgramRequestNumber:
      handleSongProgramRequest((PetalMessageAction)value);
    break;
    case queueSongRequestNumber:
      handleQueueSongRequest(value);
    break;
    default:
    break;
  }
}

void PetalMidiBridge::handleQueueSongRequest(byte position) {
  PETAL_LOGI("handleQueueSongRequest position: %d", position);
  sendAdvanceToSongMessage(position);
}

void PetalMidiBridge::sendAdvanceToSongMessage(byte position) {
  const unsigned int payloadLength = UUID_LENGTH+4;
  unsigned int encodedLength = PetalUtils::sevenBitEncodingPayloadOffset(payloadLength);
  byte payload[encodedLength];
  for (int i=0; i<UUID_LENGTH; i++) {
    payload[i] = 0;
  }
  payload[UUID_LENGTH] = DEVICE_SOURCE; // source
  payload[UUID_LENGTH+1] = REQUEST; // type
  payload[UUID_LENGTH+2] = ADVANCE_TO_SONG; // action
  payload[UUID_LENGTH+3] = position; // position

  PetalUtils::logSysExMessage("Sending Petal ADVANCE_TO_SONG Request RAW: ", payload+UUID_LENGTH, payloadLength);
  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(payload, payloadLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(payload, responseLength);
  }
}

void PetalMidiBridge::receiveSysExMessage(byte * data, unsigned length) {
  if (!isConnected()) { return; }
  // F0 + manufacturer ID
  if (length < 4) { return; }

  if (data[1] != 0x0 || data[2] != 0x0 || data[3] != MANUFACTURER_ID) {
    PETAL_LOGI("Missing manufacturer ID!");
    return;
  }
  
  if (data[length-1] != 0xF7) {
    PETAL_LOGI("Missing trailing f7!");
    return;
  }

  PETAL_LOGI("Processing SYSEX!");

  unsigned int prefixLength = 4;
  byte *messageBytes = data + prefixLength; // leave off sysex prefix
  unsigned int messageLength = length - (prefixLength + 1); // leave off sysex suffix
  if (messageLength < UUID_LENGTH + 3) {
    PETAL_LOGI("SYSEX message too short!");
    return;
  }

  // bytes is decoded in place
  PetalUtils::logBuffer("RAW MSG      ", messageBytes, messageLength);
  unsigned int decodedLength = PetalUtils::decode7BitEncodedPayload(messageBytes, length);
  PetalUtils::logBuffer("DECODED MSG  ", messageBytes, decodedLength);


  ParseMessageResponse response = parseMessage(messageBytes, messageLength);
  if (response.error == INVALID_PAYLOAD_HEADER) {
    PETAL_LOGD("PMB : dropping empty message…");
    return;
  }
  PetalUtils::logBuffer("AFTER PARSING", messageBytes, messageLength);

  PetalMessageAction action = parseAction(messageBytes);
  sendResponse(messageBytes, response);
}

void PetalMidiBridge::sendResponse(const byte *uuid, ParseMessageResponse response) {
  // send response message

  const unsigned int controlLength = 4;
  const unsigned int rawResponseLength = UUID_LENGTH+controlLength;
  byte responseBytes[rawResponseLength];
  memcpy(responseBytes, uuid, UUID_LENGTH);
  responseBytes[UUID_LENGTH] = DEVICE_SOURCE;
  responseBytes[UUID_LENGTH+1] = RESPONSE;
  responseBytes[UUID_LENGTH+2] = response.action;
  responseBytes[UUID_LENGTH+3] = response.error;
  PetalUtils::logBuffer("RESP         ", responseBytes+UUID_LENGTH, controlLength);
  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(responseBytes, rawResponseLength);
  PetalUtils::logBuffer("ENCODED RESP ", responseBytes, responseLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(responseBytes, responseLength);
  }
}

bool PetalMidiBridge::isConnected() {
  if (!interop) { return false; }
  return interop->isConnected();
}

void PetalMidiBridge::sendPetalRequest(PetalMessageAction action) {
  const unsigned int payloadLength = UUID_LENGTH+3;
  unsigned int encodedLength = PetalUtils::sevenBitEncodingPayloadOffset(payloadLength);
  byte payload[encodedLength];
  for (int i=0; i<UUID_LENGTH; i++) {
    payload[i] = 0;
  }
  payload[UUID_LENGTH] = DEVICE_SOURCE; // source
  payload[UUID_LENGTH+1] = REQUEST; // type
  payload[UUID_LENGTH+2] = action; // action

  PetalUtils::logSysExMessage("Sending Petal Request RAW: ", payload+UUID_LENGTH, payloadLength);
  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(payload, payloadLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(payload, responseLength);
  }
}

PetalProgramError PetalMidiBridge::handleProgramRequest(PetalMessage message) {
  PETAL_LOGI("creating program");
  program = new PetalSongProgram(interop, message.payload, message.payloadLength, eventHandler);
  PetalProgramError result = program->getErrorStatus();
  PETAL_LOGI("program result: %d", result);
  return result;
}

PetalProgramError PetalMidiBridge::handleProgramMessage(PetalMessage message) {
  if (program) {
    PETAL_LOGI("handleProgramMessage");
    // we have an program established and need to process a message
    program->handleMessage(message);
    PetalProgramError programError = program->getErrorStatus();
    if (message.action == UNLOAD) {
      PETAL_LOGI("UNLOAD");
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

PetalMessageAction PetalMidiBridge::parseAction(const byte * bytes) {
  return (PetalMessageAction)bytes[UUID_LENGTH+2];
}

ParseMessageResponse PetalMidiBridge::parseMessage(const byte * bytes, unsigned int length) {
  const int controlBitsLength = 3;
  if (!bytes || length < UUID_LENGTH+controlBitsLength) { 
    return ParseMessageResponse(UNKNOWN_ACTION, INVALID_PAYLOAD_HEADER);
  }

  PetalMessageSource source = (PetalMessageSource)bytes[UUID_LENGTH]; // first 16 bytes is the uuid
  PetalMessageType type = (PetalMessageType)bytes[UUID_LENGTH+1];
  PetalMessageAction action = parseAction(bytes);

  if (type != REQUEST) {
    return ParseMessageResponse(action, NO_REQUEST);
  }

  // the message payload has the uuid, source, type, and action truncated from the start

  unsigned int payloadLength = length - (UUID_LENGTH + controlBitsLength);
  byte payload[payloadLength];
  memcpy(payload, bytes + UUID_LENGTH + controlBitsLength, payloadLength);

  PetalUtils::logBuffer("MSG CONTROL", bytes+UUID_LENGTH, controlBitsLength);
  PetalUtils::logBuffer("MSG PAYLOAD", payload, payloadLength);

  PetalMessage message = PetalMessage(DEVICE_SOURCE, type, action, payload, payloadLength);

  switch (action) {
    case PROGRAM:
    case SEND_EVENTS: {
      PetalProgramError result = handleProgramRequest(message);
      return ParseMessageResponse(action, result);
    }
    default:
      return ParseMessageResponse(action, handleProgramMessage(message));
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
    event.isStartEvent = bytes[idx++];
    PETAL_LOGI("processPacket0");
    processPacket(event.packet);
    eventIndex++;
  }
}

void PetalMidiBridge::processPacket(unsigned long data) {
  if (!eventHandler) { return; }
  eventHandler->processPacket(data);
}