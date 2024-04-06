#pragma once

#include <Arduino.h>
#include <esp32-hal-log.h>

#define MANUFACTURER_ID 0x55

char * loggerFormat(String s1);

#define PETAL_LOGI(format, ...) do { char *ff = loggerFormat(format); log_i(format,  ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGD(format, ...) do { char *ff = loggerFormat(format); log_d(format,  ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGV(format, ...) do { char *ff = loggerFormat(format); log_v(format,  ##__VA_ARGS__); free(ff); } while(0)
#define PETAL_LOGE(format, ...) do { char *ff = loggerFormat(format); log_e(format,  ##__VA_ARGS__); free(ff); } while(0)

#define ULONG_SIZE sizeof(uint32_t)
#define FLOAT_SIZE sizeof(float)

void sendRemoteLoggingString(String s);
void sendRemoteLogging(char * s);

uint32_t leftShift(const byte b, const byte bits);
uint32_t parseULong(const uint8_t *programArray, int idx);
float parseFloat(const uint8_t *programArray, int idx);
uint8_t parsePacketStatus(const int data);
uint8_t parsePacketChannel(const int data);
uint8_t parsePacketNumber(const int data);
uint8_t parsePacketValue(const int data);

char * stringToCharArray(String s);

unsigned int sevenBitEncodingPayloadOffset(unsigned int length);
void encode7BitEncodedPayload(byte * payload, unsigned int length, unsigned int * encodedLength);
void decode7BitEncodedPayload(byte * payload, unsigned int length, unsigned int * decodedLength);
void logBuffer(String label, const byte* data, unsigned length);

void logSysExMessageSummary(String label, const byte* data, unsigned length);
void logSysExMessage(String label, const byte* data, unsigned length);