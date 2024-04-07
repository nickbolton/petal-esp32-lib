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
  byte message[manufacturerLength + payloadLength];
  memcpy(message, manufacturerID, manufacturerLength); 
  memcpy(message + manufacturerLength, payload, payloadLength);
  PetalUtils::logSysExMessage("TX", message, sizeof(message));
  interop->sendSysExMessage(message, sizeof(message));
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
     PETAL_LOGI("Invalid event status %02x, channel %02x, number: %02x, value: %02x", status, channel, number, value);
  }
}

void PetalEventHandler::onEventFired(float * beat) {
  byte *beatBytes = (byte *)beat;
  unsigned int encodedLength = PetalUtils::sevenBitEncodingPayloadOffset(22);
  byte payload[encodedLength] = {
    // uuid
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    // type
    NOTIFICATION,
    // action
    EVENT_FIRED,
    // beat
    beatBytes[0],  beatBytes[1],  beatBytes[2],  beatBytes[3], 
  };

  unsigned int responseLength = 0;
  PetalUtils::encode7BitEncodedPayload(payload, 22, &responseLength);
  sendSysExMessage(payload, responseLength);
}