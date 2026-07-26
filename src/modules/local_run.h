#pragma once

#include <Arduino.h>

void localRun_begin();
void localRun_loop();
bool localRun_settingsReady();
bool localRun_isActive();
bool localRun_isPaused();
int localRun_getSlot();
int localRun_getPhaseIndex();
int localRun_getDay();
float localRun_getTotalProgressPct();
float localRun_getPhaseProgressPct();
unsigned long localRun_getStartedAtSeconds();
unsigned long localRun_getRunAgeSeconds();
uint32_t localRun_getRevision();
const char* localRun_getRunId();
const char* localRun_getPresetName();
const char* localRun_getPhaseLabel();
const char* localRun_getStatusName();
bool localRun_start(int slot, unsigned long startAtSeconds, const char* runId, uint32_t revision);
bool localRun_stop();
bool localRun_pause(bool paused);
void localRun_appendJson(String& json);

