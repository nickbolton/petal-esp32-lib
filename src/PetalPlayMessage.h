#pragma once
#include <Arduino.h>
#include "PetalMessage.h"

class PetalPlayMessage {
private:
uint32_t timestamp;

public:
  PetalPlayMessage(PetalMessage message);
  uint32_t getTimestamp();
};