#include "PetalPlayMessage.h"
#include "PetalUtils.h"

PetalPlayMessage::PetalPlayMessage(PetalMessage message) {
  const byte * bytes = message.payload;
  unsigned int length = message.payloadLength;
  PetalUtils::logBuffer("PLAY BUF", bytes, length);
  
  if (!bytes || length < sizeof(unsigned long)) {
    PETAL_LOGI("ZZZ setting timestamp to zero");
    timestamp = 0;
    return;
  }
  timestamp = PetalUtils::parseULong(bytes, 0);
  PETAL_LOGI("ZZZ parsing timestamp: %04x", timestamp);
}

unsigned long PetalPlayMessage::getTimestamp() {
  return timestamp;
}