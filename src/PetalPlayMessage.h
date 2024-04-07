#pragma once
#include <Arduino.h>
#include "PetalMessage.h"

class PetalPlayMessage {
private:
unsigned long timestamp;

public:
  PetalPlayMessage(PetalMessage message);
  unsigned long getTimestamp();
};