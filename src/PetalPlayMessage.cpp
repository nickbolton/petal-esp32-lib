#include "PetalPlayMessage.h"
#include "utils.h"

PetalPlayMessage::PetalPlayMessage(PetalMessage message) {
  const byte * bytes = message.payload;
  unsigned int length = message.payloadLength;
  logBuffer("PLAY BUF", bytes, length);
  PETAL_LOGI("ZZZ length: %d, sizeof(uint32_t): %d", length, sizeof(uint32_t));
  
  if (!bytes || length < sizeof(uint32_t)) {
    PETAL_LOGI("ZZZ setting timestamp to zero");
    timestamp = 0;
    return;
  }
  timestamp = parseULong(bytes, 0);
  PETAL_LOGI("ZZZ parsing timestamp: %04x", timestamp);
}

uint32_t PetalPlayMessage::getTimestamp() {
  return timestamp;
}