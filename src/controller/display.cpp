#include "controller/display.h"

#include <Preferences.h>
#include <Wire.h>
#include <cstdio>
#include <cstring>

#include "boards/grova_core_s3.h"
#include "drivers/ssd1317.h"
#include "controller/boot_animation.h"
#include "controller/i2c_bus.h"
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
enum class Screen { Home, Menu, Detail };
static constexpr const char* menuItems[] = {"Back to Home", "Climate", "Outputs", "Fans", "Network"};
static constexpr int MenuCount = sizeof(menuItems) / sizeof(menuItems[0]);
static constexpr int LeftInset = 2;
static Screen screen = Screen::Home;
static int selection = 0;
static uint32_t drawAt = 0, probeAt = 0;
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
  probeAt = millis() - 10000;
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
  screen = Screen::Home;
  selection = 0;
  welcome.reset(millis());
  lastFrameWelcome = false;
  storageReady = displaySettings.begin("founder-oled", false);
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
void navigate(int turn, bool pressed, bool held) {
  if ((!turn && !pressed && !held) || wifiOTA_setupActive()) {
    return;
  }
  if (welcome.active(millis())) {
    welcome.dismiss();
    drawAt = millis() - 500;
    return;
  }
  if (held) {
    screen = Screen::Home;
    drawAt = millis() - 500;
    return;
  }
  switch (screen) {
    case Screen::Home:
      screen = Screen::Menu;
      selection = 1;
      break;
    case Screen::Menu:
      selection = (selection + turn + MenuCount) % MenuCount;
      if (pressed) {
        screen = selection == 0 ? Screen::Home : Screen::Detail;
      }
      break;
    case Screen::Detail:
      if (pressed) {
        screen = Screen::Menu;
      }
      break;
  }
  drawAt = millis() - 500;
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
  text(0, wide ? 44 : 100, wifiOTA_isConnected() ? "Wi-Fi on" : "No Wi-Fi");
  footer(wide ? "Press: Menu" : "Press Menu");
}

static void menu() {
  text(0, 5, "Menu");
  const bool wide = oled.width() >= 128;
  divider(13);
  for (int i = 0; i < MenuCount; ++i) {
    const int y = 15 + i * 8;
    if (i == selection) {
      founderBoot::rect(oled, LeftInset, y, oled.width() - LeftInset, 8, true);
    }
    text(0, y, i == 0 && !wide ? "Back" : menuItems[i], 1, i == selection);
  }
  footer(wide ? "Turn/Press  Hold:Home" : "Turn/Press");
}

static void detail() {
  text(0, 5, menuItems[selection]);
  divider(13);
  char value[24];
  const bool wide = oled.width() >= 128;
  switch (selection) {
    case 1:
      if (!wide) {
        reading(value, sizeof(value), SIGNAL_CLIMATE_TEMPERATURE_C, " C");
        text(0, 16, "Temp"); text(0, 25, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_HUMIDITY_PCT, " %");
        text(0, 42, "Humidity"); text(0, 51, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_PRESSURE_HPA, " hPa");
        text(0, 68, "Pressure"); text(0, 77, value);
      } else {
        reading(value, sizeof(value), SIGNAL_CLIMATE_TEMPERATURE_C, " C");
        text(0, 17, "Temperature"); text(76, 17, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_HUMIDITY_PCT, " %");
        text(0, 28, "Humidity"); text(76, 28, value);
        reading(value, sizeof(value), SIGNAL_CLIMATE_PRESSURE_HPA, " hPa");
        text(0, 39, "Pressure"); text(76, 39, value);
      }
      break;
    case 2: {
      static constexpr const char* labels[] = {"12V 1", "5V 1", "12V 2", "0-10V 1", "0-10V 2"};
      for (unsigned i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i) {
        const float percent = founderOutputs::applied(i);
        if (founder::OUTPUTS[i].analog) {
          snprintf(value, sizeof(value), "%.1f V", percent / 10);
        } else {
          snprintf(value, sizeof(value), "%s", percent > 0 ? "On" : "Off");
        }
        const int y = 15 + i * (wide ? 8 : 18);
        text(0, y, labels[i]);
        text(wide ? 65 : 0, wide ? y : y + 8, value);
      }
      break;
    }
    case 3:
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
    case 4:
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

void loop() {
  if (!ready && millis() - probeAt >= 10000) {
    probeAt = millis();
    founderBus::Scope bus;
    if (bus) {
      uint8_t address = founderBus::probe(0x3C) ? 0x3C : founderBus::probe(0x3D) ? 0x3D : 0;
      activeAddress = address;
      if (address) {
        ready = oled.begin(address, rowOffset, panel->profile);
        if (ready) {
          welcome.found(millis());
        }
      }
    }
  }
  const bool showingWelcome = welcome.active(millis());
  const uint32_t interval = showingWelcome ? founderBoot::FrameMs : lastFrameWelcome ? 0 : 500;
  if (!ready || millis() - drawAt < interval) {
    return;
  }
  drawAt = millis();
  founderBus::Scope bus;
  if (!bus) {
    return;
  }
  if (!founderBus::probe(activeAddress)) {
    ready = false;
    return;
  }
  oled.clearDisplay();
  if (showingWelcome) {
    founderBoot::draw(oled, welcome.elapsed(millis()), LeftInset);
    lastFrameWelcome = true;
    if (!oled.display()) {
      ready = false;
    }
    return;
  }
  lastFrameWelcome = false;
  if (wifiOTA_setupActive()) {
    setupScreen();
  } else {
    switch (screen) {
      case Screen::Home: home(); break;
      case Screen::Menu: menu(); break;
      case Screen::Detail: detail(); break;
    }
  }
  if (!oled.display()) {
    ready = false;
  }
}
}  // namespace founderDisplay
