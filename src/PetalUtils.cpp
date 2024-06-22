#include "PetalUtils.h"

#define SYS_EX_MAX_SIZE 4096

char *  petal_loggerFormat(String s1) {
  return PetalUtils::stringToCharArray("PETAL : " + s1 + "\n");
}

void PetalUtils::setup() {
  #ifndef ESP32 
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  #endif
}

void PetalUtils::sendRemoteLoggingString(String s) {

}

void PetalUtils::sendRemoteLogging(char * s) {

}

unsigned long PetalUtils::leftShift(const byte b, const byte bits) {
  unsigned long value = (unsigned long)b;
  return value << bits;
}

unsigned long PetalUtils::parseULong(const byte *programArray, int idx) {
  unsigned long d0 = (unsigned long)programArray[idx];
  unsigned long d1 = (unsigned long)programArray[idx + 1];
  unsigned long d2 = (unsigned long)programArray[idx + 2];
  unsigned long d3 = (unsigned long)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

float PetalUtils::parseFloat(const byte *programArray, int idx) {
  unsigned long milliBeat = parseULong(programArray, idx);
  PETAL_LOGI("parseFloat millBeat: %u", milliBeat);
  return ((float)milliBeat) / 1000.0;
}

byte PetalUtils::parsePacketStatus(const int data) {
  return (uint8_t)(data >> 24);
}

byte PetalUtils::parsePacketChannel(const int data) {
  return (uint8_t)((data & 0xffffff) >> 16);
}

byte PetalUtils::parsePacketNumber(const int data) {
  return (uint8_t)((data & 0xffff) >> 8);
}

byte PetalUtils::parsePacketValue(const int data) {
  return (uint8_t)(data & 0xff);
}

char * PetalUtils::stringToCharArray(String s) {
  size_t len = s.length() + 1;
  char * result = (char *)malloc(len);
  s.toCharArray(result, len);
  return result;
}

unsigned int PetalUtils::encode7BitEncodedPayload(byte * payload, unsigned int length) {
  if (!payload || length == 0) return 0;

  unsigned int encodedLength = 0;
  byte encoded[length];

  unsigned int byteCount = 0;
  byte overflowByte = 0;

  for (unsigned int idx = 0; idx < length; idx++) {
    byte sevenBitValue = payload[idx] & 0x7F;
    byte msb = payload[idx] >> 7;
    overflowByte += (msb << byteCount);
    byteCount++;
    
    encoded[(encodedLength)++] = sevenBitValue;
    if (byteCount >= 7) {
      encoded[(encodedLength)++] = overflowByte;
      overflowByte = 0;
      byteCount = 0;
    }
  }
 
  if (byteCount > 0) {
    encoded[(encodedLength)++] = overflowByte;
  }

  // copy encoded back into the source
  memcpy(payload, encoded, encodedLength);
  return encodedLength;
}

unsigned int PetalUtils::sevenBitEncodingPayloadOffset(unsigned int length) {
  if (length == 0) return 0;

  unsigned int encodedLength = 0;
  unsigned int byteCount = 0;

  for (unsigned int idx = 0; idx < length; idx++) {
    byteCount++;
    encodedLength++;
    if (byteCount >= 7) {
      encodedLength++;
      byteCount = 0;
    }
  }
  
  if (byteCount > 0) {
    encodedLength++;
  }

  return encodedLength;
}

unsigned int PetalUtils::decode7BitEncodedPayload(byte * payload, unsigned int length) {
  if (!payload || length == 0) return 0;

  unsigned int decodedLength = 0;
  unsigned int resultCount = 0;
  unsigned int byteCount = 0;
  unsigned int overflowByteCount = 0;

  for (unsigned int index = 0; index < length; index++) {
    byteCount++;
    if (byteCount >= 8 || index == length - 1) {
      unsigned int overflowBitCount = byteCount - 1;
      for (unsigned int idx = 0; idx < overflowBitCount; idx++) {
        byte bitmask = 1 << idx;
        unsigned int shiftCount = 7 - idx;
        unsigned int resultIndex = index - (overflowBitCount - idx) - overflowByteCount;
        payload[resultIndex] += (payload[index] & bitmask) << shiftCount;
        decodedLength = resultIndex + 1;
      }
      overflowByteCount++;
      byteCount = 0;
    } else {
      // move current byte to decoded pos
      payload[resultCount++] = payload[index];
    }
  }

  // zero out the remaining buffer
  memset(payload + decodedLength, 0, length - decodedLength);
  // PETAL_LOGI("ZZZ *decodedLength: %d, reclaimed: %d", *decodedLength, length - *decodedLength);
  // logBuffer("RESULT", payload, length);
  return decodedLength;
}

void PetalUtils::logBuffer(const char *label, const byte *data, unsigned length) {
  char dataBuffer[length*4];
  int pos = 0;
  for (uint16_t i = 0; i < length; i++) {
    char dataBuf[4];
    sprintf(dataBuf, "%02x ", data[i]);
    strcpy((dataBuffer + pos), dataBuf);
    pos += strlen(dataBuf);
  }
  if (length > 0) {
    PETAL_LOGD("%s: (%d bytes) %s", label, length, dataBuffer);
  } else {
    PETAL_LOGD("%s: (%d bytes)", label, length); 
  }
}

void PetalUtils::logSysExMessageSummary(const char *label, const byte* data, unsigned length) {
  PETAL_LOGI("%s: (%d bytes)", label, length);
}

void PetalUtils::logSysExMessage(const char *label, const byte* data, unsigned length) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
  if (length <= SYS_EX_MAX_SIZE) {
    logBuffer(label, data, length);
  } else {
    logSysExMessageSummary(label, data, length);
  }
#else 
  logSysExMessageSummary(label, data, length);
#endif
}

#ifdef ESP32 
void PetalUtils::logBuffer(String label, const byte* data, unsigned length) {
  char dataBuffer[length*4];
  int pos = 0;
  for (uint16_t i = 0; i < length; i++) {
    char dataBuf[4];
    sprintf(dataBuf, "%02x ", data[i]);
    strcpy((dataBuffer + pos), dataBuf);
    pos += strlen(dataBuf);
  }
  if (length > 0) {
    PETAL_LOGD("%s: (%d bytes) %s", label, length, dataBuffer);
  } else {
    PETAL_LOGD("%s: (%d bytes)", label, length); 
  }
}

void PetalUtils::logSysExMessageSummary(String label, const byte* data, unsigned length) {
  PETAL_LOGI("%s: (%d bytes)", label, length);
}

void PetalUtils::logSysExMessage(String label, const byte* data, unsigned length) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
  if (length <= SYS_EX_MAX_SIZE) {
    logBuffer(label, data, length);
  } else {
    logSysExMessageSummary(label, data, length);
  }
#else 
  logSysExMessageSummary(label, data, length);
#endif
}
#else

void PetalUtils::logBuffer(String label, const byte* data, unsigned length) {
  char *labelArray = stringToCharArray(label);
  logBuffer(labelArray, data, length);
  free(labelArray);
}

// void PetalUtils::logBuffer(const LoggableBuffer *buffers, unsigned length) {
//   unsigned bufferCount = 0;
//   for (int i = 0; i < length; i++) {
//     bufferCount += buffers[i].length * 4;
//   }
//   char dataBuffer[bufferCount];

//   int pos = 0;
//   for (int i = 0; i < length; i++) {
//     sprintf(dataBuffer, "%s: (%d bytes) ", buffers[i].label, buffers[i].length);
//     for (int j = 0; j < buffers[i].length; j++) {
//       char dataBuf[4];
//       sprintf(dataBuf, "%02x ", buffers[i].data[j]);
//       strcpy((dataBuffer + pos), dataBuf);
//       pos += strlen(dataBuf);
//     }
//     sprintf(dataBuffer, "  ");
//   }
//   PETAL_LOGD(dataBuffer);
// }

void PetalUtils::logSysExMessageSummary(String label, const byte* data, unsigned length) {
  char *labelArray = stringToCharArray(label);
  logSysExMessageSummary(labelArray, data, length);
  free(labelArray);
}

void PetalUtils::logSysExMessage(String label, const byte* data, unsigned length) {
  char *labelArray = stringToCharArray(label);
  logSysExMessage(labelArray, data, length);
  free(labelArray);
}

#endif

int PetalUtils::findIndex(const byte *a, size_t size, int value) {
  size_t index = 0;
  while (index < size && a[index] != value) ++index;
  return (index == size ? -1 : index);
}
