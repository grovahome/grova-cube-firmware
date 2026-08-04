# I2C Discovery

The firmware scans the configured I2C bus and reports known GROVA sensor and
system module addresses through HTTP status, MQTT telemetry, and the local ESP
status page.

No I2C sensor is required. If a known module is present, it is reported as
present and its reader is initialized automatically when that reader is part of
the active firmware. If it is missing, the cube keeps running and the matching
telemetry channel remains `null`.

## Current Known I2C Modules

| GROVA module | Device | Address | Data |
| --- | --- | --- | --- |
| GROVA Climate | SHT41/SHT4x | `0x44`, `0x45` | Temperature, humidity |
| GROVA PCB v1 Climate | AHT20/AHTx0 | `0x38` | Temperature, humidity |
| GROVA Light | VEML7700 | `0x10` | Ambient light / lux |
| GROVA CO2 | SCD41 | `0x62` | CO2, temperature, humidity |
| GROVA Pressure | BME/BMP | `0x76`, `0x77` | Pressure, temperature, optional humidity |
| GROVA UV | LTR390 | `0x53` | UV index, ambient light |
| System Display | SSD1306 OLED | `0x3C`, `0x3D` | Local display |
| System RTC | DS3231/DS1307 | `0x68` | Offline time source |

## Status Shape

`GET /api/v1/status` and MQTT telemetry include:

```json
{
  "i2c": {
    "sda": 21,
    "scl": 22,
    "found_count": 2,
    "devices": [
      {
        "id": "sht4x_44",
        "name": "SHT41/SHT4x",
        "module": "GROVA Climate",
        "addr": "0x44",
        "present": true
      }
    ]
  }
}
```

Telemetry also includes a stable `environment` object. Channels without a
connected/implemented sensor are emitted as `null` so MQTT consumers and future
apps can keep one fixed schema:

```json
{
  "environment": {
    "temperature_c": 23.4,
    "humidity_pct": 58.2,
    "pressure_hpa": null,
    "co2_ppm": null,
    "lux": null,
    "uv_index": null,
    "temperature_source": "SHT41",
    "humidity_source": "SHT41",
    "pressure_source": "NONE",
    "co2_source": "NONE",
    "lux_source": "NONE",
    "uv_source": "NONE"
  }
}
```

## Sensor Priority

Temperature and humidity should prefer the best available source, not a required
one:

1. SHT41/SHT4x
2. AHT20/AHTx0
3. SCD41
4. BME280, when present and humidity is available

In the PCB/Founder firmware line the SHT41/SHT4x and AHT20/AHTx0 drivers are
compiled in by default. If a supported climate sensor is connected and
initializes successfully, it becomes the active temperature/humidity source
according to the priority above. If it is not connected, the cube continues with
the next available source.

BME/BMP pressure, VEML7700 lux, SCD41 CO2, and LTR390 UV readings are
independent optional data channels. Missing channels must not create firmware
errors unless a future user-selected automation explicitly depends on them.

The firmware is designed for boot/restart autodiscovery and periodic best-effort
rescans. Fully supported live hot-swap under load is not guaranteed yet.

## Future GROVA Boards

The current implementation detects known device addresses. Future
self-developed GROVA sensor boards should add a board identity layer, for
example with an onboard EEPROM or equivalent ID mechanism, so the firmware can
identify module type, revision, serial number, calibration data, and preferred
address without relying only on a raw I2C address.
