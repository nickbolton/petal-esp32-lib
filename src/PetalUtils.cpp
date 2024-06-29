#include "PetalUtils.h"

#ifdef PETAL_ESP32
#else
#include "mbed.h"
#include "mbed_mem_trace.h"
#endif

#define SYS_EX_MAX_SIZE 4096


bool isInfoLoggingEnabled = false;

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

void PetalUtils::enableInfoLogging() {
  isInfoLoggingEnabled = true;
  PETAL_LOGI_F("ENABLED INFO LOGGING…");
}

void PetalUtils::disableInfoLogging() {
  isInfoLoggingEnabled = false;
  PETAL_LOGI_F("DISABLED INFO LOGGING…");
}

u_int32_t PetalUtils::parseULong(const byte *programArray, int idx) {
  u_int32_t d0 = (u_int32_t)programArray[idx];
  u_int32_t d1 = (u_int32_t)programArray[idx + 1];
  u_int32_t d2 = (u_int32_t)programArray[idx + 2];
  u_int32_t d3 = (u_int32_t)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

u_int16_t PetalUtils::parseUShort(const byte *programArray, int idx) {
  u_int16_t d0 = (u_int16_t)programArray[idx];
  u_int16_t d1 = (u_int16_t)programArray[idx + 1];
  PETAL_LOGI("parseShort d0: %x, d1: %x, result: %u 0x%x", d0, d1, (d1 << 8) + d0, (d1 << 8) + d0);
  return (d1 << 8) + d0;
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
  // PETAL_LOGI("decodedLength: %d, reclaimed: %d", decodedLength, length - decodedLength);
  // logBuffer("RESULT", payload, length);
  return decodedLength;
}

void PetalUtils::logBuffer(const char *label, const byte *data, unsigned length, bool force) {
  char dataBuffer[length*4];
  int pos = 0;
  for (uint16_t i = 0; i < length; i++) {
    char dataBuf[4];
    sprintf(dataBuf, "%02x ", data[i]);
    strcpy((dataBuffer + pos), dataBuf);
    pos += strlen(dataBuf);
  }
  if (length > 0) {
    if (force) {
      PETAL_LOGD_F("%s: (%d bytes) %s", label, length, dataBuffer);
    } else {
      PETAL_LOGD("%s: (%d bytes) %s", label, length, dataBuffer);
    }
  } else {
    if (force) {
      PETAL_LOGD_F("%s: (%d bytes)", label, length); 
    } else {
      PETAL_LOGD("%s: (%d bytes)", label, length); 
    }
  }
}

void PetalUtils::logSysExMessageSummary(const char *label, const byte* data, unsigned length, bool force) {
  if (force) {
    PETAL_LOGI_F("%s: (%d bytes)", label, length);
  } else {
    PETAL_LOGI("%s: (%d bytes)", label, length);
  }
}

void PetalUtils::logSysExMessage(const char *label, const byte* data, unsigned length, bool force) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
  if (length <= SYS_EX_MAX_SIZE) {
    logBuffer(label, data, length, force);
  } else {
    logSysExMessageSummary(label, data, length, force);
  }
#else 
  logSysExMessageSummary(label, data, length);
#endif
}

#ifdef ESP32 
void PetalUtils::logBuffer(String label, const byte* data, unsigned length, bool force) {
  char dataBuffer[length*4];
  int pos = 0;
  for (uint16_t i = 0; i < length; i++) {
    char dataBuf[4];
    sprintf(dataBuf, "%02x ", data[i]);
    strcpy((dataBuffer + pos), dataBuf);
    pos += strlen(dataBuf);
  }
  if (length > 0) {
    if (force) {
      PETAL_LOGD_F("%s: (%d bytes) %s", label, length, dataBuffer);
    } else {
      PETAL_LOGD("%s: (%d bytes) %s", label, length, dataBuffer);
    }
  } else {
    if (force) {
      PETAL_LOGD_F("%s: (%d bytes)", label, length); 
    } else {
      PETAL_LOGD("%s: (%d bytes)", label, length); 
    }
  }
}

void PetalUtils::logSysExMessageSummary(String label, const byte* data, unsigned length, bool force) {
  if (force) {
    PETAL_LOGI_F("%s: (%d bytes)", label, length);
  } else {
    PETAL_LOGI("%s: (%d bytes)", label, length);
  }
}

void PetalUtils::logSysExMessage(String label, const byte* data, unsigned length, bool force) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
  if (length <= SYS_EX_MAX_SIZE) {
    logBuffer(label, data, length, force);
  } else {
    logSysExMessageSummary(label, data, length, force);
  }
#else 
  logSysExMessageSummary(label, data, length, force);
#endif
}
#else

void PetalUtils::logBuffer(String label, const byte* data, unsigned length, bool force) {
  char *labelArray = stringToCharArray(label);
  logBuffer(labelArray, data, length, force);
  free(labelArray);
}

void PetalUtils::logSysExMessageSummary(String label, const byte* data, unsigned length, bool force) {
  char *labelArray = stringToCharArray(label);
  logSysExMessageSummary(labelArray, data, length, force);
  free(labelArray);
}

void PetalUtils::logSysExMessage(String label, const byte* data, unsigned length, bool force) {
  char *labelArray = stringToCharArray(label);
  logSysExMessage(labelArray, data, length, force);
  free(labelArray);
}

#endif

int PetalUtils::findIndex(const byte *a, size_t size, int value) {
  size_t index = 0;
  while (index < size && a[index] != value) ++index;
  return (index == size ? -1 : index);
}

#ifdef ESP32
void PetalUtils::dumpMemoryStats() {
}
#else
void PetalUtils::dumpMemoryStats() {
  // allocate enough room for every thread's stack statistics
  int cnt = osThreadGetCount();
  mbed_stats_stack_t *stats = (mbed_stats_stack_t*) malloc(cnt * sizeof(mbed_stats_stack_t));

  cnt = mbed_stats_stack_get_each(stats, cnt);
  for (int i = 0; i < cnt; i++) {
    PETAL_LOGI_F("Thread: 0x%lX, Stack size: %lu / %lu", stats[i].thread_id, stats[i].max_size, stats[i].reserved_size);
  }
  free(stats);

  // Grab the heap statistics
  mbed_stats_heap_t heap_stats;
  mbed_stats_heap_get(&heap_stats);
  PETAL_LOGI_F("Heap size: %lu / %lu bytes", heap_stats.current_size, heap_stats.reserved_size);
}
#endif