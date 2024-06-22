#include "PetalEventHandler.h"
#include "PetalMessage.h"
#include "PetalUtils.h"

const byte BANK_STATUS = 0x10;
const byte PC_STATUS = 0xc0;
const byte CC_STATUS = 0xb0;

PetalEventHandler::PetalEventHandler(PetalInteroperability *interopIn) {
  interop = interopIn;
}

PetalEventHandler::~PetalEventHandler() {
  interop = nullptr;
}

void PetalEventHandler::sendSysExMessage(const byte* payload, unsigned payloadLength) {
  if (!payload || !interop) { return; }
  byte manufacturerID[] = { 0x0, 0x0, MANUFACTURER_ID }; 

  unsigned int manufacturerLength = sizeof(manufacturerID);
  unsigned int totalLength = manufacturerLength + payloadLength;
  byte message[totalLength];
  memcpy(message, manufacturerID, manufacturerLength); 
  memcpy(message + manufacturerLength, payload, payloadLength);
  PetalUtils::logSysExMessage("SYSEX PAYLOAD: ", message, totalLength);
  interop->sendSysExMessage(message, totalLength);
}

void PetalEventHandler::sendProgramChange(byte channel, byte number) {
  if (!interop) { return; }
  interop->sendProgramChange(channel, number);
}

void PetalEventHandler::sendControlChange(byte channel, byte number, byte value) {
  if (!interop) { return; }
  interop->sendControlChange(channel, number, value);
}

void PetalEventHandler::processPacket(unsigned long data) {
  byte status = data >> 24;
  byte channel = (data & 0xffffff) >> 16;
  byte number =  (data & 0xffff) >> 8;
  byte value = data & 0xff;
  if (status == BANK_STATUS) {
    sendControlChange(channel, 0, 0);
    sendControlChange(channel, 0x20, number);
    sendProgramChange(channel, value);
  } else if (status == PC_STATUS) {
    sendProgramChange(channel, number);
  } else if (status == CC_STATUS) {
    sendControlChange(channel, number, value);
  } else {
     PETAL_LOGI("Invalid event status 0x%x, channel %d, number: 0x%x, value: 0x%x", status, channel, number, value);
  }
}

void PetalEventHandler::onEventFired(float beat, byte noteCount, byte noteValue) {
  const unsigned int beatLength = ULONG_SIZE;
  const unsigned int signatureLength = 2;
  const unsigned int controlLength = 3;
  const unsigned int totalPayloadLength = UUID_LENGTH + controlLength + beatLength + signatureLength;
  unsigned int encodedLength = PetalUtils::sevenBitEncodingPayloadOffset(totalPayloadLength);
  unsigned long milliBeat = (unsigned long)(beat * 1000.0);

  byte payload[encodedLength];
  for (int i=0; i<UUID_LENGTH; i++) {
    payload[i] = 0;
  }
  payload[UUID_LENGTH] = DEVICE_SOURCE; // source
  payload[UUID_LENGTH+1] = NOTIFICATION; // type
  payload[UUID_LENGTH+2] = EVENT_FIRED; // action
  payload[UUID_LENGTH+3] = noteCount; // signature
  payload[UUID_LENGTH+4] = noteValue; 

  memcpy(payload+UUID_LENGTH+controlLength+signatureLength, &milliBeat, beatLength);

  char beatStr[1024];
  sprintf(beatStr, "%f", beat);
  Serial.print("onEventFired beat1: ");
  Serial.print(beatStr);
  Serial.print(" milliBeat: ");
  sprintf(beatStr, "%u", milliBeat);
  Serial.println(beatStr);

  PETAL_LOGI("onEventFired noteCount: %u", noteCount);
  PETAL_LOGI("onEventFired noteValue: %u", noteValue);
  PetalUtils::logBuffer("onEventFired: ", payload+UUID_LENGTH, controlLength+signatureLength+beatLength);

  unsigned int responseLength = PetalUtils::encode7BitEncodedPayload(payload, totalPayloadLength);
  sendSysExMessage(payload, responseLength);
}