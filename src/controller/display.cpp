#include "controller/display.h"

#include <Preferences.h>
#include <Wire.h>
#include <cstdio>
#include <cstring>

#include "boards/grova_core_s3.h"
#include "drivers/ssd1317.h"
#include "controller/app.h"
#include "controller/boot_animation.h"
#include "controller/equipment.h"
#include "controller/i2c_bus.h"
#include "controller/output_test.h"
#include "controller/outputs.h"
#include "modules/fan.h"
#include "modules/rest_mode.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
#include "modules/wifi_ota.h"
namespace founderDisplay {
static GrovaSSD1317 oled;
static Preferences displaySettings;
static bool storageReady = false;
static unsigned rotate = 0, rowOffset = 80;
unsigned rotation() {
  return rotate;
}
unsigned offset() {
  return rowOffset;
}
struct Panel {
  const char* name;
  const char* apiName;
  GrovaOledProfile profile;
  unsigned defaultOffset;
};
static constexpr Panel panels[] = {
    {"SSD1317", "ssd1317", GrovaOledProfile::Ssd1317, 80},
    {"SSD1306", "ssd1306", GrovaOledProfile::Ssd1306, 0},
    {"SSD1312", "ssd1312", GrovaOledProfile::Ssd1312, 0},
    {"SH1107", "sh1107", GrovaOledProfile::Sh1107, 96},
};
static const Panel* panel = &panels[0];

const char* controller() {
  return panel->name;
}

static const Panel* findPanel(const char* name) {
  for (const Panel& candidate : panels) {
    if (!strcmp(candidate.apiName, name)) {
      return &candidate;
    }
  }
  return nullptr;
}
static bool ready = false;
static uint8_t activeAddress = 0;
static founderBoot::Sequence welcome;
static bool lastFrameWelcome = false;
bool present() {
  return ready;
}
enum class Screen {
  Home, Menu, Detail, Operation, ConfirmOperation, Devices, Device, Settings,
  IdleSettings, Brightness, Service, TestPorts, TestLevel
};
enum class Detail { Climate, Outputs, Fans, Network };
static constexpr const char* menuItems[] = {"Back to Home", "Climate", "Operation", "Devices", "Settings", "Service"};
static constexpr const char* settingsItems[] = {"Back", "Idle Mode", "Brightness", "Network"};
static constexpr const char* serviceItems[] = {"Back", "Output Test", "Outputs", "Fans"};
static constexpr const char* brightnessItems[] = {"Back", "Low", "Medium", "High"};
static constexpr int MenuCount = sizeof(menuItems) / sizeof(menuItems[0]);
static constexpr int LeftInset = 2;
static Screen screen = Screen::Home;
static int selection = 0, idleSelection = 0, testSelection = 0, testPercent = 0;
static int subSelection = 0, deviceSelection = 0, confirmSelection = 0;
static uint8_t brightness = 2;
static bool brightnessPending = false;
static Screen detailBack = Screen::Menu;
static Detail detailPage = Detail::Climate;
static uint32_t reviewRevision = 0;
static bool reviewPause = false, draftPending = false, timedStopPending = false, deviceRejected = false;
static unsigned deviceIndex = 0, draftPercent = 0;

using founderEquipment::LocalDevice;
using founderEquipment::LocalMode;
constexpr unsigned DeviceCount = founderEquipment::LocalDeviceCount;
struct DeviceList {
  const char* names[DeviceCount + 1] = {"Back"};
  unsigned indices[DeviceCount] = {};
  int count = 1;
};
static DeviceList devices(bool automatedOnly = false) {
  DeviceList list;
  for (unsigned index = 0; index < DeviceCount; ++index) {
    LocalDevice device{};
    if (founderEquipment::localDevice(index, device) &&
        (!automatedOnly || device.mode != LocalMode::Manual)) {
      list.indices[list.count - 1] = index;
      list.names[list.count++] = device.name;
    }
  }
  return list;
}
static bool testRejected = false, idleSaveFailed = false, panelOff = false;
enum class IdleMode : uint8_t { Off, Grova, AlwaysOn };
static IdleMode idleMode = IdleMode::Off;
static constexpr uint32_t IdleTimeoutMs = 60000;
static uint32_t drawAt = 0, probeAt = 0, lastInputAt = 0;
static constexpr const char* idleItems[] = {"Back", "Screen Off", "GROVA Saver", "Always On"};
static constexpr const char* portLabels[] = {"12V 1", "5V 1", "12V 2", "0-10V 1", "0-10V 2"};
static constexpr const char* testItems[] = {"Back", portLabels[0], portLabels[1], portLabels[2],
    portLabels[3], portLabels[4], "Fan 1", "Fan 2"};
static_assert(sizeof(testItems) / sizeof(testItems[0]) == founderOutputTest::PortCount + 1,
              "Every output must have a list entry");
static struct {
  char source[40] = {};
  uint32_t sampledAt = 0;
  bool valid = false;
} climate;
static bool apply(unsigned rotation, unsigned offset, const Panel* selected) {
  if (!selected || rotation > 3 || offset > oledMaxOffset(selected->profile) || !storageReady) {
    return false;
  }
  uint16_t packed = static_cast<uint16_t>(selected->profile) | (rotation << 8) | offset;
  if (displaySettings.putBytes("panel", &packed, sizeof(packed)) != sizeof(packed)) {
    return false;
  }
  welcome.dismiss();
  panel = selected;
  rotate = rotation;
  rowOffset = offset;
  oled.setRotation(rotate);
  ready = false;
  lastInputAt = millis();
  probeAt = lastInputAt - 10000;
  return true;
}
bool configure(unsigned rotation, unsigned offset, const char* profile) {
  return apply(rotation, offset, profile ? findPanel(profile) : panel);
}

bool selectController(const char* name) {
  const Panel* selected = findPanel(name);
  return selected && apply(rotate, selected->defaultOffset, selected);
}
void begin() {
  ready = false;
  activeAddress = 0;
  founderOutputTest::stop();
  screen = Screen::Home;
  selection = 0;
  climate = {};
  panelOff = false;
  lastInputAt = millis();
  welcome.reset(lastInputAt);
  lastFrameWelcome = false;
  storageReady = displaySettings.begin("founder-oled", false);
  uint8_t idle = 0;
  idleMode = IdleMode::Off;
  if (storageReady && displaySettings.getBytesLength("idle") == sizeof(idle) &&
      displaySettings.getBytes("idle", &idle, sizeof(idle)) == sizeof(idle) &&
      idle <= static_cast<uint8_t>(IdleMode::AlwaysOn)) {
    idleMode = static_cast<IdleMode>(idle);
  }
  brightness = 2;
  uint8_t storedBrightness = 2;
  if (storageReady && displaySettings.getBytesLength("brightness") == sizeof(storedBrightness) &&
      displaySettings.getBytes("brightness", &storedBrightness, sizeof(storedBrightness)) ==
          sizeof(storedBrightness) && storedBrightness <= 2) {
    brightness = storedBrightness;
  }
  brightnessPending = false;
  panel = &panels[0];
  rotate = 0;
  rowOffset = panel->defaultOffset;
  uint16_t packed = 0;
  if (storageReady && displaySettings.getBytesLength("panel") == sizeof(packed) &&
      displaySettings.getBytes("panel", &packed, sizeof(packed)) == sizeof(packed) &&
      (packed & 0x3C00) == 0) {
    const auto profile = static_cast<GrovaOledProfile>(packed & 0xC000);
    const unsigned offset = packed & 0xFF;
    for (const Panel& candidate : panels) {
      if (profile == candidate.profile && offset <= oledMaxOffset(profile)) {
        panel = &candidate;
        rotate = (packed >> 8) & 3;
        rowOffset = offset;
        break;
      }
    }
  }
  oled.setRotation(rotate);
  probeAt = millis() - 10000;
  drawAt = millis() - 1000;
}
static bool idleDue(uint32_t now) {
  return idleMode != IdleMode::AlwaysOn && now - lastInputAt >= IdleTimeoutMs;
}

static int cycle(int current, int turn, int count) {
  return (current + turn % count + count) % count;
}

static bool testPortAssigned() {
  return testSelection > 0 && founderOutputs::assigned(testSelection - 1);
}

static void navigateTest(int turn, bool pressed) {
  const int step = testSelection <= 3 ? 100 : 10;
  const int target = constrain(testPercent + constrain(turn, -1, 1) * step, 0, 100);
  if (founderOutputTest::running()) {
    if (pressed) {
      founderOutputTest::stop();
      testPercent = 0;
    } else if (turn && target != testPercent && founderOutputTest::setLevel(target)) {
      testPercent = target;
    }
    return;
  }
  if (turn) {
    testRejected = false;
    testPercent = target;
  }
  if (pressed && testPercent == 0) {
    screen = Screen::TestPorts;
  } else if (pressed && ready && founderOutputTest::available() && !testPortAssigned()) {
    testRejected = !founderOutputTest::start(testSelection - 1, testPercent);
  }
}

static void showDetail(Detail page, Screen back) {
  detailPage = page;
  detailBack = back;
  screen = Screen::Detail;
}

static void confirmOperation(int turn, bool pressed) {
  if (reviewRevision != founderEquipment::localRevision() ||
      reviewPause == restMode_isEnabled() || founderOutputTest::running()) {
    screen = Screen::Operation;
    subSelection = 0;
    return;
  }
  const int confirm = reviewPause ? 2 : devices(true).count + 1;
  confirmSelection = constrain(confirmSelection + turn, 0, confirm);
  if (!pressed) return;
  if (confirmSelection == confirm) {
    deviceRejected = !ready || !founderApp::setRest(reviewPause);
    screen = Screen::Operation;
    subSelection = 0;
  } else if (confirmSelection == 0) {
    screen = Screen::Operation;
  }
}

static void navigateDevice(int turn, bool pressed) {
  LocalDevice device{};
  if (reviewRevision != founderEquipment::localRevision() ||
      !founderEquipment::localDevice(deviceIndex, device)) {
    deviceSelection = 0;
    screen = Screen::Devices;
    return;
  }
  if (device.timed) {
    if (turn) {
      timedStopPending = turn > 0 && device.percent != 0;
      deviceRejected = false;
    }
    if (pressed && timedStopPending && ready && device.percent) {
      deviceRejected = !(device.mode == LocalMode::Manual
          ? founderEquipment::stopLocalTimed(deviceIndex) : founderApp::setRest(true));
      timedStopPending = false;
    } else if (pressed) {
      screen = Screen::Devices;
    }
    return;
  }
  if (turn) {
    const int current = draftPending ? draftPercent : device.percent;
    const int step = device.variable ? 10 : 100;
    const int target = constrain(current + constrain(turn, -1, 1) * step, 0, 100);
    draftPercent = target;
    draftPending = draftPercent != device.percent;
    deviceRejected = false;
  }
  if (pressed && draftPending) {
    deviceRejected = !ready || !founderEquipment::setLocalLevel(deviceIndex, draftPercent);
    if (!deviceRejected) draftPending = false;
  } else if (pressed && device.overrideSeconds) {
    founderEquipment::cancelLocalOverride(deviceIndex);
  } else if (pressed) {
    screen = Screen::Devices;
  }
}

void navigate(int turn, bool pressed, bool held) {
  if ((!turn && !pressed && !held) || wifiOTA_setupActive()) {
    return;
  }
  const uint32_t now = millis();
  const bool waking = idleDue(now) || panelOff;
  lastInputAt = now;
  drawAt = now - 500;
  if (waking || welcome.active(now)) {
    if (waking && screen == Screen::ConfirmOperation) screen = Screen::Operation;
    if (waking && screen == Screen::Device) {
      draftPending = false;
      timedStopPending = false;
    }
    welcome.dismiss();
    return;
  }
  if (held) {
    founderOutputTest::stop();
    screen = Screen::Home;
    return;
  }
  switch (screen) {
    case Screen::Home:
      screen = Screen::Menu;
      selection = 1;
      break;
    case Screen::Menu:
      selection = cycle(selection, turn, MenuCount);
      if (pressed) {
        subSelection = 0;
        deviceRejected = false;
        switch (selection) {
          case 0: screen = Screen::Home; break;
          case 1: showDetail(Detail::Climate, Screen::Menu); break;
          case 2: screen = Screen::Operation; break;
          case 3: deviceSelection = 0; screen = Screen::Devices; break;
          case 4: screen = Screen::Settings; break;
          case 5: screen = Screen::Service; break;
        }
      }
      break;
    case Screen::Detail:
      if (pressed) screen = detailBack;
      break;
    case Screen::Operation:
      if (turn) deviceRejected = false;
      subSelection = cycle(subSelection, turn, 2);
      if (pressed && subSelection) {
        reviewRevision = founderEquipment::localRevision();
        reviewPause = !restMode_isEnabled();
        confirmSelection = 0;
        screen = Screen::ConfirmOperation;
      } else if (pressed) {
        screen = Screen::Menu;
      }
      break;
    case Screen::ConfirmOperation:
      confirmOperation(turn, pressed);
      break;
    case Screen::Devices: {
      const auto list = devices();
      deviceSelection = cycle(deviceSelection, turn, list.count);
      if (pressed && deviceSelection) {
        deviceIndex = list.indices[deviceSelection - 1];
        reviewRevision = founderEquipment::localRevision();
        draftPending = timedStopPending = deviceRejected = false;
        screen = Screen::Device;
      } else if (pressed) {
        screen = Screen::Menu;
      }
      break;
    }
    case Screen::Device:
      navigateDevice(turn, pressed);
      break;
    case Screen::Settings:
      subSelection = cycle(subSelection, turn, 4);
      if (pressed) {
        switch (subSelection) {
          case 0: screen = Screen::Menu; break;
          case 1:
            idleSelection = static_cast<int>(idleMode) + 1;
            idleSaveFailed = false;
            screen = Screen::IdleSettings;
            break;
          case 2:
            idleSelection = brightness + 1;
            idleSaveFailed = false;
            screen = Screen::Brightness;
            break;
          case 3: showDetail(Detail::Network, Screen::Settings); break;
        }
      }
      break;
    case Screen::Brightness:
      idleSelection = cycle(idleSelection, turn, 4);
      if (pressed && idleSelection == 0) {
        screen = Screen::Settings;
      } else if (pressed) {
        const uint8_t choice = idleSelection - 1;
        if (choice != brightness) {
          idleSaveFailed = !storageReady ||
              displaySettings.putBytes("brightness", &choice, sizeof(choice)) != sizeof(choice);
          if (!idleSaveFailed) {
            brightness = choice;
            brightnessPending = true;
          }
        }
      }
      break;
    case Screen::Service:
      subSelection = cycle(subSelection, turn, 4);
      if (pressed) {
        switch (subSelection) {
          case 0: screen = Screen::Menu; break;
          case 1: testSelection = 0; screen = Screen::TestPorts; break;
          case 2: showDetail(Detail::Outputs, Screen::Service); break;
          case 3: showDetail(Detail::Fans, Screen::Service); break;
        }
      }
      break;
    case Screen::IdleSettings:
      idleSelection = cycle(idleSelection, turn, 4);
      if (pressed) {
        if (idleSelection == 0) {
          screen = Screen::Settings;
          break;
        }
        const uint8_t choice = idleSelection - 1;
        if (choice != static_cast<uint8_t>(idleMode)) {
          idleSaveFailed = !storageReady ||
              displaySettings.putBytes("idle", &choice, sizeof(choice)) != sizeof(choice);
          if (!idleSaveFailed) {
            idleMode = static_cast<IdleMode>(choice);
          }
        }
      }
      break;
    case Screen::TestPorts:
      testSelection = cycle(testSelection, turn, founderOutputTest::PortCount + 1);
      if (pressed) {
        testRejected = false;
        testPercent = 0;
        screen = testSelection == 0 ? Screen::Service : Screen::TestLevel;
      }
      break;
    case Screen::TestLevel:
      navigateTest(turn, pressed);
      break;
  }
}

static void text(int x, int y, const char* label, int size = 1, bool inverted = false) {
  // The fixed GFX font uses a 6x8 cell. Bound each label to its own row.
  x += LeftInset;
  char line[22];
  snprintf(line, sizeof(line), "%.*s", (oled.width() - x) / (6 * size), label);
  oled.setTextSize(size);
  oled.setTextColor(inverted ? 0 : 1);
  oled.setCursor(x, y);
  oled.print(static_cast<const char*>(line));
}

static void divider(int y) {
  founderBoot::rect(oled, LeftInset, y, oled.width() - LeftInset, 1, true);
}

static void footer(const char* label) {
  divider(oled.height() - 11);
  text(0, oled.height() - 8, label);
}

static void reading(char* out, size_t length, SignalId id, const char* unit) {
  const auto value = signalRegistry_read(id);
  if (value.valid) {
    snprintf(out, length, "%.0f%s", value.value, unit);
  } else {
    snprintf(out, length, "--%s", unit);
  }
}

static void updateClimate() {
  const auto reading = signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C);
  climate.valid = reading.valid;
  if (reading.valid) {
    climate.sampledAt = reading.updatedAtMs;
    snprintf(climate.source, sizeof(climate.source), "%s", reading.source);
  }
}

static void climateInfo(bool wide) {
  if (!climate.source[0]) {
    text(0, 16, "Waiting");
    text(wide ? 48 : 0, wide ? 16 : 26, "for sensor");
    return;
  }
  char source[22];
  char status[22];
  const char* port = strchr(climate.source, '@');
  const int length = port ? port - climate.source : strlen(climate.source);
  snprintf(source, sizeof(source), "%.*s", length, climate.source);
  if (port && !strncmp(port, "@ch", 3) && port[3] >= '0' && port[3] <= '7') {
    if (wide) {
      snprintf(source, sizeof(source), "%.*s / Port %c", length, climate.source, port[3]);
    } else {
      snprintf(status, sizeof(status), "Port %c", port[3]);
      text(0, 26, status);
    }
  }
  text(0, 16, source);
  const auto seconds = uint32_t(millis() - climate.sampledAt) / 1000;
  const char* label = !climate.valid ? "Offline" : seconds >= 4 ? "Last" : "Age";
  snprintf(status, sizeof(status), "%s %lus", label, static_cast<unsigned long>(seconds));
  text(wide ? 48 : 0, wide ? 5 : 36, status);
}

static void home() {
  const bool wide = oled.width() >= 128;
  const bool paused = restMode_isEnabled();
  for (unsigned i = 0; i < 5; ++i) {
    founderBoot::letter(oled, i, LeftInset + i * 6, 5, 1);
  }
  const int badgeX = wide ? 85 : 0;
  const int badgeY = wide ? 3 : 18;
  founderBoot::rect(oled, badgeX + LeftInset, badgeY, 40, 11, paused);
  text(badgeX + 3, badgeY + 2, paused ? "PAUSED" : "ACTIVE", 1, paused);
  divider(wide ? 16 : 34);

  char value[22];
  if (isTimeSynced()) {
    snprintf(value, sizeof(value), "%02d:%02d", getHour(), getMinute());
  } else {
    snprintf(value, sizeof(value), "--:--");
  }
  text(0, wide ? 25 : 43, value, 2);
  const int climateX = wide ? 75 : 0;
  const int climateY = wide ? 21 : 72;
  reading(value, sizeof(value), SIGNAL_CLIMATE_TEMPERATURE_C, " C");
  text(climateX, climateY, value);
  reading(value, sizeof(value), SIGNAL_CLIMATE_HUMIDITY_PCT, " %");
  text(climateX, climateY + 10, value);
  if (!climate.valid && climate.source[0]) {
    text(0, wide ? 44 : 100, wide ? "Sensor offline" : "Sens. off");
  } else if (paused) {
    text(0, wide ? 44 : 100, wide ? "Schedule paused" : "Paused");
  } else {
    const auto action = founderEquipment::nextAction();
    if (action.name) {
      char target[6];
      if (action.pulse || !action.percent || !action.variable) {
        snprintf(target, sizeof(target), "%s", action.pulse ? "Run" : action.percent ? "On" : "Off");
      } else {
        snprintf(target, sizeof(target), "%u%%", action.percent);
      }
      if (wide) {
        snprintf(value, sizeof(value), "%02u:%02u %s %.*s", action.minute / 60, action.minute % 60,
                 target, 14 - int(strlen(target)), action.name);
      } else {
        snprintf(value, sizeof(value), "%02u:%02u %s", action.minute / 60, action.minute % 60, target);
      }
      text(0, wide ? 44 : 100, value);
    } else {
      text(0, wide ? 44 : 100, wifiOTA_isConnected() ? "Wi-Fi on" : "No Wi-Fi");
    }
  }
  footer(wide ? "Press: Menu" : "Press Menu");
}

static void list(const char* title, const char* const* entries, int count, int selected,
                 int saved = -1) {
  text(0, 5, title);
  divider(13);
  const int rows = (oled.height() - 23) / 8;
  const int maximumStart = count > rows ? count - rows : 0;
  const int start = constrain(selected - rows / 2, 0, maximumStart);
  for (int i = start; i < count && i < start + rows; ++i) {
    const int y = 14 + (i - start) * 8;
    if (i == selected) {
      founderBoot::rect(oled, LeftInset, y, oled.width() - LeftInset, 7, true);
    }
    text(0, y, entries[i], 1, i == selected);
    if (i == saved) {
      text(oled.width() - 8, y, "*", 1, i == selected);
    }
  }
}

static void menu() {
  const bool wide = oled.width() >= 128;
  static constexpr const char* narrow[] = {"Back", "Climate", "Operation", "Devices", "Settings", "Service"};
  list("Menu", wide ? menuItems : narrow, MenuCount, selection);
  footer(wide ? "Turn/Press  Hold:Home" : "Turn/Press");
}

static const char* modeLabel(LocalMode mode) {
  switch (mode) {
    case LocalMode::Manual: return "Manual";
    case LocalMode::Scheduled: return "Schedule";
    case LocalMode::Sensor: return "Sensor";
  }
  return "";
}

static void operation() {
  const bool paused = restMode_isEnabled();
  const char* actions[] = {"Back", paused ? "Resume" : "Pause all"};
  list("Operation", actions, 2, subSelection);
  text(0, 37, paused ? "PAUSED" : "ACTIVE");
  footer(deviceRejected ? "Action blocked" : "Turn/Press");
}

static void operationReview() {
  if (reviewPause) {
    static constexpr const char* entries[] = {"Cancel", "All outputs off", "Confirm"};
    list("Pause all", entries, 3, confirmSelection);
  } else {
    const auto automated = devices(true);
    const bool wide = oled.width() >= 128;
    const char* entries[DeviceCount + 3] = {"Cancel", "Enable outputs"};
    char rows[DeviceCount][22] = {};
    for (int i = 1; i < automated.count; ++i) {
      LocalDevice device{};
      founderEquipment::localDevice(automated.indices[i - 1], device);
      if (wide) {
        snprintf(rows[i - 1], sizeof(rows[i - 1]), "%s %.11s", modeLabel(device.mode), device.name);
      } else {
        snprintf(rows[i - 1], sizeof(rows[i - 1]), "%c %.8s",
                 device.mode == LocalMode::Scheduled ? 'T' : 'S', device.name);
      }
      entries[i + 1] = rows[i - 1];
    }
    entries[automated.count + 1] = "Confirm";
    list("Resume", entries, automated.count + 2, confirmSelection);
  }
  footer("Review/OK");
}

static void deviceList() {
  const auto listData = devices();
  list("Devices", listData.names, listData.count, constrain(deviceSelection, 0, listData.count - 1));
  if (listData.count == 1) text(0, 29, "No devices");
  footer("Turn/Press");
}

static void deviceScreen() {
  LocalDevice device{};
  if (!founderEquipment::localDevice(deviceIndex, device) ||
      reviewRevision != founderEquipment::localRevision()) {
    text(0, 12, "Config changed");
    footer("Press Back");
    return;
  }
  text(0, 5, device.name);
  divider(13);
  char line[22];
  const bool wide = oled.width() >= 128;
  if (device.timed) {
    text(0, 17, device.percent ? "RUNNING" : "STOPPED");
    text(0, 29, modeLabel(device.mode));
    if (timedStopPending) {
      text(0, 40, device.mode == LocalMode::Manual ? "Stop device" : (wide ? "Pause all to stop" : "Pause all"));
    } else if (device.percent) {
      text(0, 40, wide ? "Turn to stop" : "Turn: Stop");
    }
    footer(deviceRejected ? "Stop error" : timedStopPending ? "Press Stop" : "Press Back");
    return;
  }
  const unsigned level = draftPending ? draftPercent : device.percent;
  if (device.variable) {
    snprintf(line, sizeof(line), "%u%%", level);
  } else {
    snprintf(line, sizeof(line), "%s", level ? "ON" : "OFF");
  }
  text(0, 17, line, 2);
  if (device.overrideSeconds) {
    snprintf(line, sizeof(line), wide ? "Override %u:%02u left" : "Ov. %u:%02u",
             device.overrideSeconds / 60, device.overrideSeconds % 60);
    text(0, 37, line);
  } else if (draftPending && device.mode != LocalMode::Manual) {
    text(0, 37, wide ? "Override for 15 min" : "15 min");
  } else {
    text(0, 37, modeLabel(device.mode));
  }
  footer(deviceRejected ? "Blocked" : draftPending ? "Press Save"
      : device.overrideSeconds ? "Press Auto" : wide ? "Turn: Set  Press:Back" : "Turn/Back");
}

static void brightnessScreen() {
  list("Brightness", brightnessItems, 4, idleSelection, brightness + 1);
  footer(idleSaveFailed ? "Save error" : "Press Save");
}

static void idleSettings() {
  const bool wide = oled.width() >= 128;
  static constexpr const char* narrow[] = {"Back", "Screen Off", "GROVA", "Always On"};
  list(wide ? "Idle after 1 min" : "Idle 1 min", wide ? idleItems : narrow, 4, idleSelection,
       static_cast<int>(idleMode) + 1);
  footer(idleSaveFailed ? "Save error" : wide ? "Press: Save   * Saved" : "Press Save");
}

static void testPorts() {
  list("Output Test", testItems, founderOutputTest::PortCount + 1, testSelection);
  footer("Turn/Press");
}

static void testLevel() {
  const bool wide = oled.width() >= 128;
  text(0, 5, testItems[testSelection]);
  divider(13);
  char value[22];
  if (testSelection <= 3) {
    snprintf(value, sizeof(value), "%s", testPercent ? "ON" : "OFF");
  } else if (testSelection <= 5) {
    snprintf(value, sizeof(value), "%.1f V", testPercent / 10.0);
  } else {
    snprintf(value, sizeof(value), "%d%%", testPercent);
  }
  text(0, 17, value, 2);
  if (founderOutputTest::running()) {
    snprintf(value, sizeof(value), "%us left", (founderOutputTest::remainingMs() + 999) / 1000);
    text(0, 38, value);
    footer(wide ? "Turn/Press Stop" : "Press Stop");
  } else if (testPortAssigned()) {
    text(0, 36, "Assigned");
    text(0, 45, wide ? "Unassign in web UI" : "Use web UI");
    footer(wide ? "Hold: Home" : "Hold Home");
  } else if (!founderOutputTest::available()) {
    text(0, 38, restMode_isEnabled() ? (wide ? "Test unavailable" : "Locked")
                                    : (wide ? "Pause via Operation" : "Pause Core"));
    footer("Hold Home");
  } else {
    text(0, 38, testRejected ? "Test error" : wide ? "Turn to set level" : "Turn level");
    footer(testPercent ? "Press Run" : "Press Back");
  }
}

static void detail() {
  static constexpr const char* titles[] = {"Climate", "Outputs", "Fans", "Network"};
  text(0, 5, titles[static_cast<unsigned>(detailPage)]);
  divider(13);
  char value[24];
  const bool wide = oled.width() >= 128;
  switch (detailPage) {
    case Detail::Climate:
      climateInfo(wide);
      if (!wide) {
        reading(value, sizeof(value), SIGNAL_CLIMATE_TEMPERATURE_C, " C");
        text(0, 48, "Temp"); text(0, 57, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_HUMIDITY_PCT, " %");
        text(0, 69, "Humidity"); text(0, 78, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_PRESSURE_HPA, " hPa");
        text(0, 90, "Pressure"); text(0, 99, value);
      } else {
        reading(value, sizeof(value), SIGNAL_CLIMATE_TEMPERATURE_C, " C");
        text(0, 25, "Temperature"); text(76, 25, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_HUMIDITY_PCT, " %");
        text(0, 34, "Humidity"); text(76, 34, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_PRESSURE_HPA, " hPa");
        text(0, 43, "Pressure"); text(76, 43, value);
      }
      break;
    case Detail::Outputs: {
      text(wide ? 48 : 0, wide ? 5 : 106, "Setpoints");
      for (unsigned i = 0; i < sizeof(portLabels) / sizeof(portLabels[0]); ++i) {
        const float percent = founderOutputs::applied(i);
        if (founder::OUTPUTS[i].analog) {
          snprintf(value, sizeof(value), "%.1f V", percent / 10);
        } else {
          snprintf(value, sizeof(value), "%s", percent > 0 ? "On" : "Off");
        }
        const int y = 15 + i * (wide ? 8 : 18);
        text(0, y, portLabels[i]);
        text(wide ? 65 : 0, wide ? y : y + 8, value);
      }
      break;
    }
    case Detail::Fans:
      if (wide) {
        snprintf(value, sizeof(value), "Fan 1 %d%% / %d RPM", getFanPercent(), getFanRPM());
        text(0, 17, value);
        snprintf(value, sizeof(value), "Fan 2 %d%% / %d RPM", getFan2Percent(), getFan2RPM());
        text(0, 29, value);
      } else {
        snprintf(value, sizeof(value), "Fan 1 %d%%", getFanPercent());
        text(0, 17, value);
        snprintf(value, sizeof(value), "%d RPM", getFanRPM());
        text(0, 26, value);
        snprintf(value, sizeof(value), "Fan 2 %d%%", getFan2Percent());
        text(0, 44, value);
        snprintf(value, sizeof(value), "%d RPM", getFan2RPM());
        text(0, 53, value);
      }
      break;
    case Detail::Network:
      if (wifiOTA_isConnected()) {
        text(0, 17, wide ? "Wi-Fi connected" : "Wi-Fi OK");
        text(0, 29, wide ? "Open in browser" : "Web IP");
        const char* ip = wifiOTA_getIP();
        if (wide) {
          text(0, 40, ip);
        } else {
          const char* firstDot = strchr(ip, '.');
          const char* secondDot = firstDot ? strchr(firstDot + 1, '.') : nullptr;
          snprintf(value, sizeof(value), "%.*s", secondDot ? int(secondDot - ip + 1) : 15, ip);
          text(0, 41, value);
          if (secondDot) text(0, 50, secondDot + 1);
        }
      } else {
        text(0, 17, "No Wi-Fi");
      }
      break;
  }
  footer(wide ? "Press: Back Hold:Home" : "Press Back");
}

static void setupScreen() {
  const char* lines[] = {"GROVA Wi-Fi Setup", wifiOTA_setupSSID(), wifiOTA_setupPassword(),
                         wifiOTA_setupIP(), wifiOTA_setupState(),
                         wifiOTA_isConnected() ? wifiOTA_getIP() : ""};
  const size_t width = (oled.width() - LeftInset) / 6;
  int y = 5;
  for (const char* line : lines) {
    for (size_t i = 0, length = strlen(line); i < length; i += width) {
      if (y + 8 > oled.height()) {
        return;
      }
      text(0, y, line + i);
      y += 8;
    }
  }
}

static int bounce(uint32_t travel, int limit) {
  const uint32_t step = travel % (2 * limit);
  return step <= unsigned(limit) ? step : 2 * limit - step;
}

static void saver(uint32_t idleMs) {
  const int scale = oled.width() >= 128 ? 2 : 1;
  const int x = bounce(idleMs / 85, oled.width() - 29 * scale);
  const int y = bounce(idleMs / 112, oled.height() - 7 * scale);
  for (unsigned i = 0; i < 5; ++i) {
    founderBoot::letter(oled, i, x + i * 6 * scale, y, scale);
  }
}

void loop() {
  founderOutputTest::loop();
  updateClimate();
  const bool setup = wifiOTA_setupActive();
  if (setup) {
    lastInputAt = millis();
  }
  if (!ready && millis() - probeAt >= 10000) {
    probeAt = millis();
    founderBus::Scope bus;
    if (bus) {
      uint8_t address = founderBus::probe(0x3C) ? 0x3C : founderBus::probe(0x3D) ? 0x3D : 0;
      activeAddress = address;
      if (address) {
        ready = oled.begin(address, rowOffset, panel->profile);
        if (ready) {
          panelOff = false;
          brightnessPending = brightness != 2;
          welcome.found(millis());
        }
      }
    }
  }
  const uint32_t now = millis();
  const bool idle = !setup && idleDue(now);
  const bool screenOff = idle && idleMode == IdleMode::Off;
  const bool showingWelcome = welcome.active(now);
  const uint32_t interval = showingWelcome ? founderBoot::FrameMs :
                            lastFrameWelcome ? 0 : idle ? 250 : 500;
  if (!ready || (screenOff && panelOff) || (now - drawAt < interval && !panelOff)) {
    return;
  }
  drawAt = now;
  founderBus::Scope bus;
  if (!bus || !founderBus::probe(activeAddress)) {
    founderOutputTest::stop();
    if (bus) {
      ready = false;
    }
    return;
  }
  if (screenOff) {
    panelOff = oled.setPower(false);
    ready = panelOff;
    founderOutputTest::stop();
    return;
  }

  if (brightnessPending) {
    if (!oled.setContrast(brightness)) {
      ready = false;
      founderOutputTest::stop();
      return;
    }
    brightnessPending = false;
  }
  oled.clearDisplay();
  lastFrameWelcome = showingWelcome;
  if (showingWelcome) {
    founderBoot::draw(oled, welcome.elapsed(now), LeftInset);
  } else if (idle) {
    saver(now - lastInputAt - IdleTimeoutMs);
  } else if (setup) {
    setupScreen();
  } else {
    switch (screen) {
      case Screen::Home: home(); break;
      case Screen::Menu: menu(); break;
      case Screen::Detail: detail(); break;
      case Screen::Operation: operation(); break;
      case Screen::ConfirmOperation: operationReview(); break;
      case Screen::Devices: deviceList(); break;
      case Screen::Device: deviceScreen(); break;
      case Screen::Settings:
        list("Settings", settingsItems, 4, subSelection);
        footer("Turn/Press");
        break;
      case Screen::Brightness: brightnessScreen(); break;
      case Screen::Service: {
        static constexpr const char* narrow[] = {"Back", "Out Test", "Outputs", "Fans"};
        list("Service", oled.width() >= 128 ? serviceItems : narrow, 4, subSelection);
        footer("Turn/Press");
        break;
      }
      case Screen::IdleSettings: idleSettings(); break;
      case Screen::TestPorts: testPorts(); break;
      case Screen::TestLevel: testLevel(); break;
    }
  }
  if (!oled.display() || (panelOff && !oled.setPower(true))) {
    ready = false;
    founderOutputTest::stop();
  }
  panelOff = false;
}
}  // namespace founderDisplay
