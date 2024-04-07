#include "PetalUtils.h"

#define SYS_EX_MAX_SIZE 4096

char *  petal_loggerFormat(String s1) {
  return PetalUtils::stringToCharArray("PETAL : " + s1 + "\n");
}

void PetalUtils::sendRemoteLoggingString(String s) {

}

void  PetalUtils::sendRemoteLogging(char * s) {

}

unsigned long  PetalUtils::leftShift(const byte b, const byte bits) {
  unsigned long value = (unsigned long)b;
  return value << bits;
}

unsigned long  PetalUtils::parseULong(const byte *programArray, int idx) {
  unsigned long d0 = (unsigned long)programArray[idx];
  unsigned long d1 = (unsigned long)programArray[idx + 1];
  unsigned long d2 = (unsigned long)programArray[idx + 2];
  unsigned long d3 = (unsigned long)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

float  PetalUtils::parseFloat(const byte *programArray, int idx) {
  return (float)programArray[idx];
}

byte  PetalUtils::parsePacketStatus(const int data) {
  return (uint8_t)(data >> 24);
}

byte  PetalUtils::parsePacketChannel(const int data) {
  return (uint8_t)((data & 0xffffff) >> 16);
}

byte  PetalUtils::parsePacketNumber(const int data) {
  return (uint8_t)((data & 0xffff) >> 8);
}

byte  PetalUtils::parsePacketValue(const int data) {
  return (uint8_t)(data & 0xff);
}

char *  PetalUtils::stringToCharArray(String s) {
  size_t len = s.length() + 1;
  char * result = (char *)malloc(len);
  s.toCharArray(result, len);
  return result;
}

void  PetalUtils::encode7BitEncodedPayload(byte * payload, unsigned int length, unsigned int * encodedLength) {
  *encodedLength = 0;

  if (!payload || length == 0) {
    return;
  }

  byte* encoded = (byte *)malloc(length);

  unsigned int byteCount = 0;
  byte overflowByte = 0;

  for (unsigned int idx = 0; idx < length; idx++) {
    byte sevenBitValue = payload[idx] & 0x7F;
    byte msb = payload[idx] >> 7;
    overflowByte += (msb << byteCount);
    byteCount++;
    
    encoded[(*encodedLength)++] = sevenBitValue;
    if (byteCount >= 7) {
      encoded[(*encodedLength)++] = overflowByte;
      overflowByte = 0;
      byteCount = 0;
    }
  }
  
  if (byteCount > 0) {
    encoded[(*encodedLength)++] = overflowByte;
  }

  // copy encoded back into the source
  memcpy(payload, encoded, *encodedLength);
  free(encoded);
}

unsigned int  PetalUtils::sevenBitEncodingPayloadOffset(unsigned int length) {
  unsigned int encodedLength = 0;

  if (length == 0) {
    return encodedLength;
  }

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

void  PetalUtils::decode7BitEncodedPayload(byte * payload, unsigned int length, unsigned int * decodedLength) {
  *decodedLength = 0;

  if (!payload || length == 0) {
    return;
  }

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
        *decodedLength = resultIndex + 1;
      }
      overflowByteCount++;
      byteCount = 0;
    } else {
      // move current byte to decoded pos
      payload[resultCount++] = payload[index];
    }
  }

  // zero out the remaining buffer
  memset(payload + *decodedLength, 0, length - *decodedLength);
  // PETAL_LOGI("ZZZ *decodedLength: %d, reclaimed: %d", *decodedLength, length - *decodedLength);
  // logBuffer("RESULT", payload, length);
}

void  PetalUtils::logBuffer(String label, const byte* data, unsigned length) {
  char* dataBuffer = (char *)malloc(length*4);
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
  free(dataBuffer);
}

void  PetalUtils::logSysExMessageSummary(String label, const byte* data, unsigned length) {
  PETAL_LOGI("%s: (%d bytes)", label, length);
}

void  PetalUtils::logSysExMessage(String label, const byte* data, unsigned length) {
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