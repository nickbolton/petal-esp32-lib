#pragma once

#include <Arduino.h>
#ifdef ESP32
#include <esp32-hal-log.h>
#else
#include <ArduinoLog.h>
#endif

char *  petal_loggerFormat(String s1);

#define MANUFACTURER_ID 0x55

#ifdef PETAL_ESP32
#define PETAL_LOGI(format, ...) do { char *ff = petal_loggerFormat(format); log_i(format,  ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGD(format, ...) do { char *ff = petal_loggerFormat(format); log_d(format,  ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGV(format, ...) do { char *ff = petal_loggerFormat(format); log_v(format,  ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGE(format, ...) do { char *ff = petal_loggerFormat(format); log_e(format,  ##__VA_ARGS__); free(ff); } while(0)
#else
// #define PETAL_LOGI(format, ...) do { char *ff = petal_loggerFormat(format); char buff[sizeof(format)+1]; sprintf(buff, format, ##__VA_ARGS__); Serial.println(buff); free(ff); } while(0)
// #define PETAL_LOGD(format, ...) do { char *ff = petal_loggerFormat(format); char buff[sizeof(format)+1]; sprintf(buff, format, ##__VA_ARGS__); Serial.println(buff); free(ff); } while(0)
// #define PETAL_LOGV(format, ...) do { char *ff = petal_loggerFormat(format); char buff[sizeof(format)+1]; sprintf(buff, format, ##__VA_ARGS__); Serial.println(buff); free(ff); } while(0)
// #define PETAL_LOGE(format, ...) do { char *ff = petal_loggerFormat(format); char buff[sizeof(format)+1]; sprintf(buff, format, ##__VA_ARGS__); Serial.println(buff); free(ff); } while(0)
#define PETAL_LOGI(format, ...) do { char *ff = petal_loggerFormat(format); Log.noticeln(format, ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGD(format, ...) do { char *ff = petal_loggerFormat(format); Log.traceln(format, ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGV(format, ...) do { char *ff = petal_loggerFormat(format); Log.verboseln(format, ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGE(format, ...) do { char *ff = petal_loggerFormat(format); Log.errorln(format, ##__VA_ARGS__); free(ff); } while(0)

#endif

#define ULONG_SIZE sizeof(uint32_t)
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

  static unsigned long leftShift(const byte b, const byte bits);
  static unsigned long parseULong(const byte *programArray, int idx);
  static float parseFloat(const byte *programArray, int idx);
  static byte parsePacketStatus(const int data);
  static byte parsePacketChannel(const int data);
  static byte parsePacketNumber(const int data);
  static byte parsePacketValue(const int data);

  static char * stringToCharArray(String s);

  static unsigned int sevenBitEncodingPayloadOffset(unsigned int length);
  static unsigned int encode7BitEncodedPayload(byte * payload, unsigned int length);
  static unsigned int decode7BitEncodedPayload(byte * payload, unsigned int length);

  static void logBuffer(String label, const byte* data, unsigned length);
  static void logSysExMessageSummary(String label, const byte* data, unsigned length);
  static void logSysExMessage(String label, const byte* data, unsigned length);
  static void logBuffer(const char *label, const byte *data, unsigned length);
  // static void logBuffer(const LoggableBuffer *buffers, unsigned length);
  static void logSysExMessageSummary(const char *label, const byte* data, unsigned length);
  static void logSysExMessage(const char *label, const byte* data, unsigned length);

  static int findIndex(const byte *a, size_t size, int value);
};