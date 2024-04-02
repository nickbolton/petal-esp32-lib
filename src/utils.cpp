#include "utils.h"
#include <Arduino.h>

bool isDebugLogging = false;
bool isTraceLogging = false;

static const char* LOG_TAG = "Petal";

void sendRemoteLoggingString(String s) {

}

void sendRemoteLogging(char * s) {

}

uint32_t leftShift(const byte b, const byte bits) {
  uint32_t value = (uint32_t)b;
  return value << bits;
}

uint32_t parseULong(const uint8_t *programArray, int idx) {
  uint32_t d0 = (uint32_t)programArray[idx];
  uint32_t d1 = (uint32_t)programArray[idx + 1];
  uint32_t d2 = (uint32_t)programArray[idx + 2];
  uint32_t d3 = (uint32_t)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

float parseFloat(const uint8_t *programArray, int idx) {
  return (float)programArray[idx];
}

uint8_t parsePacketStatus(const int data) {
  return (uint8_t)(data >> 24);
}

uint8_t parsePacketChannel(const int data) {
  return (uint8_t)((data & 0xffffff) >> 16);
}

uint8_t parsePacketNumber(const int data) {
  return (uint8_t)((data & 0xffff) >> 8);
}

uint8_t parsePacketValue(const int data) {
  return (uint8_t)(data & 0xff);
}

char * stringToCharArray(String s) {
  size_t len = s.length() + 1;
  char * result = (char *)malloc(len);
  s.toCharArray(result, len);
  return result;
}

char * loggerFormat(String s1) {
  return stringToCharArray("PETAL : " + s1 + "\n");
  // const char* prefix = "PETAL : ";
  // const char* suffix = "\n";
  // char* result = (char *)malloc(strlen(prefix)+strlen(s1)+strlen(suffix)+1);
  // strcpy(result, prefix);
  // strcat(result, s1);
  // strcat(result, suffix);
  // return result;
}

void encode7BitEncodedPayload(byte * payload, unsigned int length, unsigned int * encodedLength) {
  *encodedLength = 0;

  if (!payload || length == 0) {
    return;
  }

  byte encoded[length];

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
}

unsigned int sevenBitEncodingPayloadOffset(unsigned int length) {
  unsigned int encodedLength = 0;

  if (length == 0) {
    return encodedLength;
  }

  unsigned int byteCount = 0;
  byte overflowByte = 0;

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

void decode7BitEncodedPayload(byte * payload, unsigned int length, unsigned int * decodedLength) {
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

void logBuffer(String label, const byte* data, unsigned length) {
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