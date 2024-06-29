#pragma once

#include <Arduino.h>
#ifdef ESP32
#include <esp32-hal-log.h>
#else
#include <ArduinoLog.h>
#endif

extern bool isInfoLoggingEnabled;

char *  petal_loggerFormat(String s1);

#define MANUFACTURER_ID 0x55

#define PETAL_LOGI(format, ...)       do { if (isInfoLoggingEnabled) { char *ff = petal_loggerFormat(format); Serial.printf(format, ##__VA_ARGS__); free(ff); }} while(0)
#define PETAL_LOGD(format, ...)       do { if (isInfoLoggingEnabled) { char *ff = petal_loggerFormat(format); Serial.printf(format, ##__VA_ARGS__); free(ff); }} while(0)
#define PETAL_LOGV(format, ...)       do { if (isInfoLoggingEnabled) { char *ff = petal_loggerFormat(format); Serial.printf(format, ##__VA_ARGS__); free(ff); }} while(0)
#define PETAL_LOGI_F(format, ...) do { char *ff = petal_loggerFormat(format); Serial.printf(format, ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGD_F(format, ...) do { char *ff = petal_loggerFormat(format); Serial.printf(format, ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGE(format, ...)       do { char *ff = petal_loggerFormat(format); Serial.printf(format, ##__VA_ARGS__); free(ff); } while(0)

#define ULONG_SIZE sizeof(uint32_t)
#define USHORT_SIZE sizeof(uint16_t)
#define FLOAT_SIZE sizeof(uint32_t)

// struct LoggableBuffer {
//   const char* label;
//   const byte* data;
//   unsigned length;
// };

struct PetalUtils {
   
  public:
  static void setup();
  static void sendRemoteLoggingString(String s);
  static void sendRemoteLogging(char * s);

  static u_int32_t parseULong(const byte *programArray, int idx);
  static u_int16_t parseUShort(const byte *programArray, int idx);
  static float parseFloat(const byte *programArray, int idx);
  static byte parsePacketStatus(const int data);
  static byte parsePacketChannel(const int data);
  static byte parsePacketNumber(const int data);
  static byte parsePacketValue(const int data);

  static char * stringToCharArray(String s);

  static unsigned int sevenBitEncodingPayloadOffset(unsigned int length);
  static unsigned int encode7BitEncodedPayload(byte * payload, unsigned int length);
  static unsigned int decode7BitEncodedPayload(byte * payload, unsigned int length);

  static void logBuffer(String label, const byte* data, unsigned length, bool force);
  static void logSysExMessageSummary(String label, const byte* data, unsigned length, bool force);
  static void logSysExMessage(String label, const byte* data, unsigned length, bool force);
  static void logBuffer(const char *label, const byte *data, unsigned length, bool force);
  static void logSysExMessageSummary(const char *label, const byte* data, unsigned length, bool force);
  static void logSysExMessage(const char *label, const byte* data, unsigned length, bool force);

  static int findIndex(const byte *a, size_t size, int value);
  static void enableInfoLogging();
  static void disableInfoLogging();
  static void dumpMemoryStats();
};