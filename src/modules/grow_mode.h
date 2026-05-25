#pragma once

enum GrowMode {
  GROW_MODE_GERMINATION,
  GROW_MODE_GROWTH,
  GROW_MODE_HARVEST,
  GROW_MODE_COUNT
};

void growMode_begin();
GrowMode growMode_get();
bool growMode_set(GrowMode mode, bool saveNow);
const char* growMode_getName();
const char* growMode_getEffectName();
bool growMode_settingsReady();
bool growMode_isGermination();
bool growMode_isGrowth();
bool growMode_isHarvest();
void growMode_adjust(int delta);
void growMode_save();
