#pragma once

#include <Arduino.h>

#include "config.h"

constexpr uint8_t GROVA_PRESET_SLOT_COUNT = GROVA_LOCAL_PRESET_SLOTS;
constexpr uint8_t GROVA_PRESET_MAX_PHASES = GROVA_LOCAL_PRESET_MAX_PHASES;
constexpr uint8_t GROVA_PRESET_MAX_PUMP_EVENTS = GROVA_LOCAL_PRESET_MAX_PUMP_EVENTS;

#pragma pack(push, 1)
struct LocalPresetPumpEvent {
  uint8_t enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t duration_s;
};

struct LocalPresetPhase {
  char key[16];
  char label[24];
  uint16_t duration_days;
  int16_t day_temp_x10;
  int16_t night_temp_x10;
  uint8_t day_hum_pct;
  uint8_t night_hum_pct;
  uint16_t temp_warn_delta_x10;
  uint8_t hum_warn_delta_pct;
  uint8_t light_enabled;
  uint8_t light_on_hour;
  uint8_t light_off_hour;
  uint8_t pump_enabled;
  uint8_t pump_event_count;
  LocalPresetPumpEvent pump_events[GROVA_PRESET_MAX_PUMP_EVENTS];
};

struct LocalPresetBlob {
  uint32_t magic;
  uint8_t version;
  uint8_t phase_count;
  char id[48];
  char name[32];
  uint32_t updated_at_s;
  LocalPresetPhase phases[GROVA_PRESET_MAX_PHASES];
  uint32_t checksum;
};
#pragma pack(pop)

void presetStore_begin();
bool presetStore_settingsReady();
int presetStore_getActiveSlot();
bool presetStore_setActiveSlot(int slot);
bool presetStore_clearSlot(int slot);
bool presetStore_saveHexPayload(int slot, const String& payloadHex);
bool presetStore_loadSlot(int slot, LocalPresetBlob& preset);
void presetStore_appendSummaryJson(String& json);
void presetStore_appendFullJson(String& json);

