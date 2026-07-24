#pragma once

// Copy this file to include/secrets.h and enter your local credentials there.
// include/secrets.h is ignored by Git.
// Hardware profile, sensors, and pins belong in include/board_config.h.
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

#define MQTT_HOST "192.168.1.94"
#define MQTT_PORT 1883
#define MQTT_USER "grova"
#define MQTT_PASS "your-mqtt-password"

// Use a unique ID for every physical cube. MQTT topics, backend storage,
// history, dashboard selection, and commands are separated by this value.
#define MQTT_CUBE_ID "grova-cube-001"

// Optional Home Assistant / dashboard display name published through MQTT discovery.
#define MQTT_DEVICE_NAME "GROVA Cube 1"
