#include "PetalMidiBridge.h"
#include "PetalUtils.h"

const int controlBitsLength = 3;

byte incomingMidiChannel = 16;
const byte songProgramRequestNumber = 101;
const byte queueSongRequestNumber = 102;

long lastMessageTime = 0;

PetalMidiBridge::PetalMidiBridge(PetalInteroperability *interopIn) {
  interop = interopIn;
  eventHandler = new PetalEventHandler(interop);
  resetMultiparts();
}

PetalMidiBridge::~PetalMidiBridge() {
  tearDownProgram();
  if (eventHandler) {
    delete eventHandler;
  }
  eventHandler = nullptr;
  resetMultiparts();
}

void PetalMidiBridge::tearDownProgram() {
  for (short index = 0; index < songPrograms.size(); index++) {
    delete songPrograms[index];
  }
  songPrograms.clear();
  songIDToIndexMap.clear();
}

unsigned short PetalMidiBridge::getSongIndex() {
  return songIndex;
}

void PetalMidiBridge::setSongIndex(unsigned short index) {
  songIndex = index;
}

PetalSongProgram* PetalMidiBridge::currentSongProgram() {
  if (songIndex >= songPrograms.size()) { return nullptr; }
  return songPrograms[songIndex];
}

void PetalMidiBridge::setup() {
  PETAL_LOGI("Setting up petal bridge…");
}

void PetalMidiBridge::process() {
  PetalSongProgram *program = currentSongProgram();
  if (program) {
    program->process();
  }
  // long now = millis();
  // if (now - lastMessageTime >= 2000) {
  //   byte bytes[] = {1,2,3,4,5};
  //   sendResponse(bytes, VALID_REQUEST);
  //   lastMessageTime = now;
  // }
}

// CC Message Handling

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

void PetalMidiBridge::handleSongProgramRequest(PetalMessageAction action) {
  PETAL_LOGI("handleSongProgramRequest program %x, action: %d", songPrograms, action);
  PetalSongProgram *program = currentSongProgram();
  if (!program) { return; }
  sendPetalRequest(action);
  switch (action) {
    case PLAY:
      if (program->getProgramStatus() == RUNNING) {
        program->handleMessage(PetalMessage(MIDI_SOURCE, REQUEST, PAUSE));
        break;
      }
      // fallthrough
    case STOP:
      program->handleMessage(PetalMessage(MIDI_SOURCE, REQUEST, action));
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
  memset(payload, 0, UUID_LENGTH);
  payload[UUID_LENGTH] = DEVICE_SOURCE; // source
  payload[UUID_LENGTH+1] = REQUEST; // type
  payload[UUID_LENGTH+2] = ADVANCE_TO_SONG; // action
  payload[UUID_LENGTH+3] = position; // position

  PetalUtils::logSysExMessage("Sending Petal ADVANCE_TO_SONG Request RAW: ", payload+UUID_LENGTH, payloadLength, false);
  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(payload, payloadLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(payload, responseLength);
  }
}

// SYS EX Message Handling

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
  PetalUtils::logBuffer("RAW MSG      ", messageBytes, messageLength, false);
  unsigned int decodedLength = PetalUtils::decode7BitEncodedPayload(messageBytes, messageLength);
  PetalUtils::logBuffer("DECODED MSG  ", messageBytes, decodedLength, false);

  ParseMessageResponse response = parseMessage(messageBytes, messageLength);
  if (response.error == INVALID_PAYLOAD_HEADER) {
    PETAL_LOGD("PMB : dropping empty message…");
    return;
  }
  PetalUtils::logBuffer("AFTER PARSING", messageBytes, messageLength, false);

  sendResponse(messageBytes, response);
}

ParseMessageResponse PetalMidiBridge::parseMessage(const byte * bytes, unsigned int length) {
  if (!bytes || length < UUID_LENGTH+controlBitsLength) { 
    return ParseMessageResponse(UNKNOWN_ACTION, INVALID_PAYLOAD_HEADER);
  }

  PetalMessageType type = (PetalMessageType)bytes[UUID_LENGTH+1]; // first 16 bytes is the uuid
  PetalMessageAction action = parseAction(bytes);

  if (type != REQUEST) {
    return ParseMessageResponse(action, NO_REQUEST);
  }

  if (action == MULTI_PART) {
    return parseMultiPartMessage(action, bytes, length);
  } else {
    return parseRawMessage(type, action, bytes, length);
  }
}

ParseMessageResponse PetalMidiBridge::parseMultiPartMessage(PetalMessageAction action, const byte * bytes, unsigned int length) {
  u_int32_t uuid = PetalUtils::parseULong(bytes, 0);
  if (uuid == 0) {
    resetMultiparts();
  }
  MultiPartMeta meta; 
  multiPartMeta.find(uuid);
  if (multiPartMeta.find(uuid) != multiPartMeta.end()) {
    meta = multiPartMeta[uuid];
  } else {
    meta = startMultiPart(bytes);
  }

  u_int8_t partNumber = parseMultiPartNumber(bytes);
  u_int8_t partCount = parseMultiPartCount(bytes);
  u_int16_t partSize = parseMultiPartSize(bytes);
  u_int16_t totalSize = parseMultiPartTotalSize(bytes);
  const byte *payload = parseMultiPartPayload(bytes);

  memcpy(meta.payload + meta.payloadLength, payload, partSize);
  meta.payloadLength += partSize;
  multiPartMeta[uuid] = meta;

  PETAL_LOGI("RECEIVED MULTIPART uuid(0x%x) %u of %u, size: %u, %u of %u size", uuid, partNumber, partCount, partSize, meta.payloadLength, totalSize);
  PetalUtils::logBuffer("MULTIPART PART PAYLOAD", payload, partSize, false);
  PetalUtils::logBuffer("MULTIPART ACCUMULATED PAYLOAD", meta.payload, meta.payloadLength, false);

  if (partNumber == partCount) {
    if (meta.payloadLength == totalSize) {
      if (meta.payloadLength < UUID_LENGTH+2) {
        PETAL_LOGI("ABORTING MULTIPART uuid(0x%x), INVALID SIZE", uuid);
        clearMultiPart(uuid);
        return ParseMessageResponse(action, INVALID_MULTI_PART);
      }
      PetalUtils::logBuffer("MULTIPART FULL MESSAGE", meta.payload, meta.payloadLength, false);
      PetalMessageAction subAction = parseAction(meta.payload);
      PETAL_LOGI("MULTIPART SUB TYPE: %u", meta.payload[UUID_LENGTH+1]);
      PETAL_LOGI("MULTIPART SUB ACTION: 0x%x", subAction);
      if (subAction == MULTI_PART) {
        PETAL_LOGI("ABORTING MULTIPART uuid(0x%x), EMBEDDED MULTIPART", uuid);
        clearMultiPart(uuid);
        return ParseMessageResponse(action, INVALID_MULTI_PART);
      }
      ParseMessageResponse response = parseMessage(meta.payload, meta.payloadLength);
      clearMultiPart(uuid);
      return response;
    } else {
      PETAL_LOGI("ABORTING MULTIPART uuid(0x%x), INCORRECT TOTAL RECEIVED", uuid);
      clearMultiPart(uuid);
      return ParseMessageResponse(action, INVALID_MULTI_PART);
    }
  } else if (partNumber > partCount) {
    PETAL_LOGI("ABORTING MULTIPART uuid(0x%x), MORE PARTS THAN SPECIFIED", uuid);
    clearMultiPart(uuid);
    return ParseMessageResponse(action, INVALID_MULTI_PART);
  } else if (meta.payloadLength >= totalSize) {
    PETAL_LOGI("ABORTING MULTIPART uuid(0x%x), PAYLOAD TOTAL REACHED BEFORE END", uuid);
    clearMultiPart(uuid);
    return ParseMessageResponse(action, INVALID_MULTI_PART);
  } else {
    return ParseMessageResponse(action, VALID_REQUEST);
  }
}

ParseMessageResponse PetalMidiBridge::parseRawMessage(PetalMessageType type, PetalMessageAction action, const byte * bytes, unsigned int length) {

  // the message payload has the uuid, source, type, and action truncated from the start

  unsigned int payloadLength = length - (UUID_LENGTH + controlBitsLength);
  byte payload[payloadLength];
  memcpy(payload, bytes + UUID_LENGTH + controlBitsLength, payloadLength);

  PetalUtils::logBuffer("MSG CONTROL", bytes+UUID_LENGTH, controlBitsLength, false);
  PetalUtils::logBuffer("MSG PAYLOAD", payload, payloadLength, false);

  PetalMessage message = PetalMessage(DEVICE_SOURCE, type, action, payload, payloadLength);

  PETAL_LOGI_F("PetalMidiBridge ACTION: %u", action);

  PetalProgramError result = NO_REQUEST;

  switch (action) {
    case PROGRAM:
    case SEND_EVENTS:
      result = handleProgramRequest(message);
      break;
    case UNLOAD:
      result = unloadProgram();
      break;
    case ACTIVATE:
      result = handleProgramActivate(message);
      break;
    case DEBUGGING:
      result = handleDebuggingEnabled(message);
      break;
    default:
      result = handleProgramMessage(message);
      break;
  }

  PETAL_LOGI_F("PetalMidiBridge RESULT: %u", result);

  PetalUtils::dumpMemoryStats();
  return ParseMessageResponse(action, result);
}

// Helpers

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
  PetalUtils::logBuffer("RESP         ", responseBytes+UUID_LENGTH, controlLength, false);
  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(responseBytes, rawResponseLength);
  PetalUtils::logBuffer("ENCODED RESP ", responseBytes, responseLength, false);
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
  memset(payload, 0, UUID_LENGTH);
  payload[UUID_LENGTH] = DEVICE_SOURCE; // source
  payload[UUID_LENGTH+1] = REQUEST; // type
  payload[UUID_LENGTH+2] = action; // action

  PetalUtils::logSysExMessage("Sending Petal Request RAW: ", payload+UUID_LENGTH, payloadLength, false);
  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(payload, payloadLength);
  if (eventHandler) {
    eventHandler->sendSysExMessage(payload, responseLength);
  }
}

PetalProgramError PetalMidiBridge::handleProgramRequest(PetalMessage message) {
  PETAL_LOGI("creating program");
  
  int idx = 0;
  activateFirstSong = (bool)message.payload[idx++];
  u_int8_t programCount = message.payload[idx++];

  PETAL_LOGI("programCount: %u", programCount);

  for (u_int8_t i = 0; i < programCount; i++) {
    u_int32_t programPayloadLength = PetalUtils::parseULong(message.payload, idx);
    idx += ULONG_SIZE;
    PETAL_LOGI("program %d: payloadLength: %u", i, programPayloadLength);
    PetalSongProgram *program = new PetalSongProgram(interop, message.payload + idx, programPayloadLength, eventHandler);
    idx += programPayloadLength;

    if (programCount == 1 && songIDToIndexMap.find(program->getSongId()) != songIDToIndexMap.end()) {
      songPrograms[songIDToIndexMap[program->getSongId()]] = program;
    } else {
      songPrograms.push_back(program);
      songIDToIndexMap[program->getSongId()] = i;
    }
  }
  songIndex = 0;
  if (programCount > 0 && activateFirstSong) {
    activatePosition(0);
  }
  
  PetalProgramError result = currentSongProgram()->getErrorStatus();
  PETAL_LOGI("program result: %d", result);
  return result;
}

PetalProgramError PetalMidiBridge::unloadProgram() {
  PETAL_LOGI("UNLOADING PROGRAM…");
  tearDownProgram();
  return VALID_REQUEST;
}

PetalProgramError PetalMidiBridge::handleProgramActivate(PetalMessage message) {
  if (message.payloadLength < 1) { return INVALID_ACTION; }
  u_int8_t position = message.payload[0];
  if (position >= songPrograms.size()) {
    PETAL_LOGI("ACTIVATE SONG %u ATTEMPT out of range(%u)", position, songPrograms.size());
    return INVALID_ACTION;
  }
  activatePosition(position);
  return VALID_REQUEST;
}

PetalProgramError PetalMidiBridge::handleDebuggingEnabled(PetalMessage message) {
  if (message.payloadLength < 1) { return INVALID_ACTION; }
  bool isLoggingEnabled = message.payload[0] != 0;
  if (isLoggingEnabled) {
    PetalUtils::enableInfoLogging();
  } else {
    PetalUtils::disableInfoLogging(); 
  }
  return VALID_REQUEST;
}

void PetalMidiBridge::activatePosition(u_int8_t position) {
  PETAL_LOGI("ACTIVATING SONG %u", position);
  currentSongProgram()->setActive(false);
  songIndex = position;
  currentSongProgram()->setActive(true);
}

PetalProgramError PetalMidiBridge::handleProgramMessage(PetalMessage message) {
  PetalSongProgram *program = currentSongProgram();
  if (program) {
    PETAL_LOGI("handleProgramMessage");
    // we have an program established and need to process a message
    program->handleMessage(message);
    PetalProgramError programError = program->getErrorStatus();
    return programError;
  } else {
    return NO_PROGRAM;
  }
}

PetalMessageAction PetalMidiBridge::parseAction(const byte * bytes) {
  return (PetalMessageAction)bytes[UUID_LENGTH+2];
}

u_int8_t PetalMidiBridge::parseMultiPartNumber(const byte * bytes) {
  return bytes[UUID_LENGTH+3];
}

u_int8_t PetalMidiBridge::parseMultiPartCount(const byte * bytes) {
  return bytes[UUID_LENGTH+4];
}

u_int16_t PetalMidiBridge::parseMultiPartSize(const byte * bytes) {
  return PetalUtils::parseUShort(bytes, UUID_LENGTH+5);
}

u_int16_t PetalMidiBridge::parseMultiPartTotalSize(const byte * bytes) {
  return PetalUtils::parseUShort(bytes, UUID_LENGTH+7);
}

const byte * PetalMidiBridge::parseMultiPartPayload(const byte * bytes) {
  return bytes + UUID_LENGTH + 9;
}

void PetalMidiBridge::clearMultiPart(u_int32_t uuid) {
  PETAL_LOGI("CLEARING MULTIPART uuid(0x%x)", uuid);
  if (multiPartMeta.find(uuid) != multiPartMeta.end()) {
    if (multiPartMeta[uuid].payload) {
      free(multiPartMeta[uuid].payload);
    }
    multiPartMeta.erase(uuid);
  }
}

void PetalMidiBridge::resetMultiparts() {
  std::map<u_int32_t, MultiPartMeta>::iterator it;
  for (it = multiPartMeta.begin(); it != multiPartMeta.end(); it++) {
    clearMultiPart(it->first);
  }
}

MultiPartMeta PetalMidiBridge::startMultiPart(const byte * bytes) {
  PetalUtils::logBuffer("STARTING MULTIPART", bytes, UUID_LENGTH, false);
  u_int32_t uuid = PetalUtils::parseULong(bytes, 0);
  u_int16_t totalLength = parseMultiPartTotalSize(bytes);
  MultiPartMeta meta;
  meta.payloadLength = 0;
  meta.payload = (byte *)malloc(totalLength);
  multiPartMeta[uuid] = meta;
  return meta;
}

void PetalMidiBridge::processPacket(unsigned long data) {
  if (!eventHandler) { return; }
  eventHandler->processPacket(data);
}