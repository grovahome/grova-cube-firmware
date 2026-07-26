#include <Arduino.h>
#include <Preferences.h>
#include <ctype.h>
#include <string.h>

#include "modules/preset_store.h"

static constexpr uint32_t PRESET_MAGIC = 0x47525053UL; // GRPS
static constexpr uint8_t PRESET_VERSION = 1;
static constexpr uint32_t FNV_OFFSET = 2166136261UL;
static constexpr uint32_t FNV_PRIME = 16777619UL;

static Preferences presetSettings;
static bool settingsReady = false;
static int activeSlot = -1;

static uint32_t checksumPreset(const LocalPresetBlob& preset) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&preset);
  const size_t length = sizeof(LocalPresetBlob) - sizeof(uint32_t);
  uint32_t hash = FNV_OFFSET;
  for (size_t i = 0; i < length; i++) {
    hash ^= bytes[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

static bool validSlot(int slot) {
  return slot >= 0 && slot < GROVA_PRESET_SLOT_COUNT;
}

static String slotKey(int slot) {
  String key = "p";
  key += slot;
  return key;
}

static int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  return -1;
}

static bool decodeHex(const String& hex, uint8_t* output, size_t outputSize) {
  if (hex.length() != outputSize * 2) return false;
  for (size_t i = 0; i < outputSize; i++) {
    int high = hexValue(hex[i * 2]);
    int low = hexValue(hex[i * 2 + 1]);
    if (high < 0 || low < 0) return false;
    output[i] = static_cast<uint8_t>((high << 4) | low);
  }
  return true;
}

static void appendEscapedJsonValue(String& json, const char* value) {
  for (const char* cursor = value; *cursor; cursor++) {
    char c = *cursor;
    if (c == '"' || c == '\\') {
      json += '\\';
      json += c;
    } else if (c == '\n') {
      json += "\\n";
    } else if (c == '\r') {
      json += "\\r";
    } else if (c == '\t') {
      json += "\\t";
    } else {
      json += c;
    }
  }
}

static void appendJsonString(String& json, const char* key, const char* value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  appendEscapedJsonValue(json, value);
  json += "\"";
  if (comma) json += ",";
}

static void appendJsonBool(String& json, const char* key, bool value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (comma) json += ",";
}

static void appendJsonInt(String& json, const char* key, long value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value;
  if (comma) json += ",";
}

static bool validatePreset(const LocalPresetBlob& preset) {
  if (preset.magic != PRESET_MAGIC || preset.version != PRESET_VERSION) return false;
  if (preset.phase_count == 0 || preset.phase_count > GROVA_PRESET_MAX_PHASES) return false;
  if (preset.checksum != checksumPreset(preset)) return false;
  for (uint8_t i = 0; i < preset.phase_count; i++) {
    const LocalPresetPhase& phase = preset.phases[i];
    if (phase.day_temp_x10 < 50 || phase.day_temp_x10 > 400) return false;
    if (phase.night_temp_x10 < 50 || phase.night_temp_x10 > 400) return false;
    if (phase.day_hum_pct > 100 || phase.night_hum_pct > 100) return false;
    if (phase.light_on_hour > 23 || phase.light_off_hour > 23) return false;
    if (phase.pump_event_count > GROVA_PRESET_MAX_PUMP_EVENTS) return false;
    for (uint8_t eventIndex = 0; eventIndex < phase.pump_event_count; eventIndex++) {
      const LocalPresetPumpEvent& event = phase.pump_events[eventIndex];
      if (event.hour > 23 || event.minute > 59 || event.minute % 5 != 0) return false;
      if (event.duration_s < 1 || event.duration_s > 10) return false;
    }
  }
  return true;
}

void presetStore_begin() {
  settingsReady = presetSettings.begin("grova-presets", false);
  if (!settingsReady) {
    Serial.println("Local presets settings unavailable");
    return;
  }
  activeSlot = presetSettings.getChar("active", -1);
  if (!validSlot(activeSlot)) activeSlot = -1;
  Serial.print("Local preset active slot: ");
  Serial.println(activeSlot);
}

bool presetStore_settingsReady() {
  return settingsReady;
}

int presetStore_getActiveSlot() {
  return activeSlot;
}

bool presetStore_loadSlot(int slot, LocalPresetBlob& preset) {
  if (!settingsReady || !validSlot(slot)) return false;
  memset(&preset, 0, sizeof(preset));
  String key = slotKey(slot);
  size_t length = presetSettings.getBytesLength(key.c_str());
  if (length != sizeof(LocalPresetBlob)) return false;
  if (presetSettings.getBytes(key.c_str(), &preset, sizeof(LocalPresetBlob)) != sizeof(LocalPresetBlob)) return false;
  return validatePreset(preset);
}

bool presetStore_setActiveSlot(int slot) {
  if (!settingsReady) return false;
  if (slot == -1) {
    activeSlot = -1;
    presetSettings.putChar("active", -1);
    return true;
  }
  LocalPresetBlob preset;
  if (!presetStore_loadSlot(slot, preset)) return false;
  activeSlot = slot;
  presetSettings.putChar("active", static_cast<int8_t>(slot));
  return true;
}

bool presetStore_clearSlot(int slot) {
  if (!settingsReady || !validSlot(slot)) return false;
  String key = slotKey(slot);
  presetSettings.remove(key.c_str());
  if (activeSlot == slot) presetStore_setActiveSlot(-1);
  return true;
}

bool presetStore_saveHexPayload(int slot, const String& payloadHex) {
  if (!settingsReady || !validSlot(slot)) return false;
  LocalPresetBlob preset;
  memset(&preset, 0, sizeof(preset));
  if (!decodeHex(payloadHex, reinterpret_cast<uint8_t*>(&preset), sizeof(LocalPresetBlob))) return false;
  if (!validatePreset(preset)) return false;
  String key = slotKey(slot);
  return presetSettings.putBytes(key.c_str(), &preset, sizeof(LocalPresetBlob)) == sizeof(LocalPresetBlob);
}

static void appendPresetSummary(String& json, int slot, bool comma) {
  LocalPresetBlob preset;
  bool saved = presetStore_loadSlot(slot, preset);
  json += "{";
  appendJsonInt(json, "slot", slot);
  appendJsonBool(json, "saved", saved);
  appendJsonBool(json, "active", slot == activeSlot);
  if (saved) {
    appendJsonString(json, "id", preset.id);
    appendJsonString(json, "name", preset.name);
    appendJsonInt(json, "phase_count", preset.phase_count);
    appendJsonInt(json, "updated_at_s", preset.updated_at_s, false);
  } else {
    appendJsonString(json, "id", "");
    appendJsonString(json, "name", "");
    appendJsonInt(json, "phase_count", 0);
    appendJsonInt(json, "updated_at_s", 0, false);
  }
  json += "}";
  if (comma) json += ",";
}

void presetStore_appendSummaryJson(String& json) {
  json += "\"local_presets\":{";
  appendJsonInt(json, "max_slots", GROVA_PRESET_SLOT_COUNT);
  appendJsonInt(json, "max_phases", GROVA_PRESET_MAX_PHASES);
  appendJsonInt(json, "max_pump_events_per_phase", GROVA_PRESET_MAX_PUMP_EVENTS);
  appendJsonInt(json, "active_slot", activeSlot);
  appendJsonBool(json, "settings_ok", settingsReady);
  json += "\"slots\":[";
  for (int slot = 0; slot < GROVA_PRESET_SLOT_COUNT; slot++) {
    appendPresetSummary(json, slot, slot < GROVA_PRESET_SLOT_COUNT - 1);
  }
  json += "]}";
}

static void appendPresetFull(String& json, const LocalPresetBlob& preset) {
  appendJsonString(json, "id", preset.id);
  appendJsonString(json, "name", preset.name);
  appendJsonInt(json, "phase_count", preset.phase_count);
  appendJsonInt(json, "updated_at_s", preset.updated_at_s);
  json += "\"phases\":[";
  for (uint8_t i = 0; i < preset.phase_count; i++) {
    const LocalPresetPhase& phase = preset.phases[i];
    json += "{";
    appendJsonString(json, "key", phase.key);
    appendJsonString(json, "label", phase.label);
    appendJsonInt(json, "duration_days", phase.duration_days);
    appendJsonInt(json, "day_temp_x10", phase.day_temp_x10);
    appendJsonInt(json, "night_temp_x10", phase.night_temp_x10);
    appendJsonInt(json, "day_hum_pct", phase.day_hum_pct);
    appendJsonInt(json, "night_hum_pct", phase.night_hum_pct);
    appendJsonInt(json, "temp_warn_delta_x10", phase.temp_warn_delta_x10);
    appendJsonInt(json, "hum_warn_delta_pct", phase.hum_warn_delta_pct);
    appendJsonBool(json, "light_enabled", phase.light_enabled);
    appendJsonInt(json, "light_on_hour", phase.light_on_hour);
    appendJsonInt(json, "light_off_hour", phase.light_off_hour);
    appendJsonBool(json, "pump_enabled", phase.pump_enabled);
    appendJsonInt(json, "pump_event_count", phase.pump_event_count);
    json += "\"pump_events\":[";
    for (uint8_t eventIndex = 0; eventIndex < phase.pump_event_count; eventIndex++) {
      const LocalPresetPumpEvent& event = phase.pump_events[eventIndex];
      json += "{";
      appendJsonBool(json, "enabled", event.enabled);
      appendJsonInt(json, "hour", event.hour);
      appendJsonInt(json, "minute", event.minute);
      appendJsonInt(json, "duration_s", event.duration_s, false);
      json += "}";
      if (eventIndex < phase.pump_event_count - 1) json += ",";
    }
    json += "]}";
    if (i < preset.phase_count - 1) json += ",";
  }
  json += "]";
}

void presetStore_appendFullJson(String& json) {
  json += "{";
  presetStore_appendSummaryJson(json);
  json += ",\"presets\":[";
  bool first = true;
  for (int slot = 0; slot < GROVA_PRESET_SLOT_COUNT; slot++) {
    LocalPresetBlob preset;
    if (!presetStore_loadSlot(slot, preset)) continue;
    if (!first) json += ",";
    json += "{";
    appendJsonInt(json, "slot", slot);
    appendJsonBool(json, "active", slot == activeSlot);
    appendPresetFull(json, preset);
    json += "}";
    first = false;
  }
  json += "]}";
}

