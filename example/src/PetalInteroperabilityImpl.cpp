#include "PetalInteroperabilityImpl.h"
#include "PetalUtils.h"
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <MIDI.h>

struct MIDISettings : public MIDI_NAMESPACE::DefaultSettings
{
  static const bool Use1ByteParsing = true;
  static const unsigned SysExMaxSize = 4096;
};

const byte BANK_STATUS = 0x10;
const byte PC_STATUS = 0xc0;
const byte CC_STATUS = 0xb0;

PetalInteroperabilityImpl *current = nullptr;

#define BLEMIDI BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>
#define PETAL_MIDI MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>, MIDISettings> 

MIDI_CREATE_INSTANCE(HardwareSerial,Serial1, MIDI_HARDWARE);
BLEMIDI _bleMidi = BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>("PetalMIDIBridge");
PETAL_MIDI _midi = MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>, MIDISettings>((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE> &)_bleMidi);

void onConnected() {
  if (!current) { return; }
  PETAL_LOGI("BLEMIDI connected!");
  current->setConnected(true);
}

void onDisconnected() {
  if (!current) { return; }
  PETAL_LOGI("BLEMIDI disconnected!");
  current->setConnected(true);
}

void onMidiSysEx(byte* data, unsigned length) {
  PETAL_LOGI("RX SYSEX!!");
  if (!current || !current->bridge() || !current->isConnected()) { return; }
  current->bridge()->receiveSysExMessage(data, length);
}

void errorCallback(int8_t error) {
  PETAL_LOGE("Received error from MIDI: %d", error);
}

void onControlChange(unsigned char channel, unsigned char number, unsigned char value) {
  if (!current || !current->bridge() || !current->isConnected()) { return; }
  PETAL_LOGD("RX CC channel %2d number %3d value %3d", channel, number, value);
}

void handleControlChange(byte channel, byte number, byte value) {
  if (!current || !current->bridge() || !current->isConnected()) { return; }
  PETAL_LOGI("Received CC %d on Arduino channel %d with value %02x", number, channel, value);
  current->bridge()->receiveControlChange(channel, number, value);
}

PetalInteroperabilityImpl::PetalInteroperabilityImpl() {
  current = this;
}

PetalInteroperabilityImpl::~PetalInteroperabilityImpl() { 
  _bridge = nullptr; 
  current = nullptr;
}

void PetalInteroperabilityImpl::setup(PetalMidiBridge *bridge) {
  _bridge = bridge;
  try {
    PETAL_LOGI("Setting up BLE MIDI…");

    _bleMidi.setHandleConnected(onConnected);
    _bleMidi.setHandleDisconnected(onDisconnected);

    _midi.setHandleControlChange(onControlChange);
    _midi.setHandleSystemExclusive(onMidiSysEx);
    _midi.setHandleError(errorCallback);
    _midi.begin();
  } catch (...) {
    PETAL_LOGE("An error occurred setting up petal bridge!");
  }

  PETAL_LOGI("Setting up hardware MIDI…");
  MIDI_HARDWARE.begin(MIDI_CHANNEL_OMNI);
  MIDI_HARDWARE.turnThruOff();
  MIDI_HARDWARE.setHandleControlChange(handleControlChange);
}

void PetalInteroperabilityImpl::process() {
  _midi.read();
}

bool PetalInteroperabilityImpl::isConnected() {
  return _isConnected;
}

void PetalInteroperabilityImpl::setConnected(bool b) {
  _isConnected = b;
}

void PetalInteroperabilityImpl::sendSysExMessage(const byte * message, unsigned length) {
  _midi.sendSysEx(sizeof(message), message, false);
}

void PetalInteroperabilityImpl::sendProgramChange(byte channel, byte number) {
  PETAL_LOGI("Sending pc %d on channel %d", number, channel);
  MIDI_HARDWARE.sendProgramChange(number, channel);
}

void PetalInteroperabilityImpl::sendControlChange(byte channel, byte number, byte value) {
  PETAL_LOGI("Sending cc %d %02x on channel %d", number, value, channel);
  MIDI_HARDWARE.sendControlChange(number, value, channel);
}

void PetalInteroperabilityImpl::processPacket(uint32_t data) {
  byte status = data >> 24;
  byte channel = (data & 0xffffff) >> 16;
  byte number =  (data & 0xffff) >> 8;
  byte value = data & 0xff;
  if (status == BANK_STATUS) {
    sendControlChange(channel, 0, 0);
    sendControlChange(channel, 0x20, number);
    sendProgramChange(channel, value);
  } else if (status == PC_STATUS) {
    sendProgramChange(channel, number);
  } else if (status == CC_STATUS) {
    sendControlChange(channel, number, value);
  } else {
     PETAL_LOGI("Invalid event status %02x, channel %02x, number: %02x, value: %02x", status, channel, number, value);
  }
}