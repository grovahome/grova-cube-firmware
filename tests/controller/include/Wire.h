#pragma once
#include <array>
#include <cstdint>
#include <vector>
struct FakeWire {
  bool sht[8] = {}, badCrc = false;
  bool oled=true,failOledData=false;
  size_t oledBytes=0;
  bool oledPowered = true;
  bool mux = true, rtc = true, failMask = false;
  int failRegister = -1;
  uint8_t mask = 0, address = 0, pointer = 0;
  std::array<uint8_t, 256> memory{};
  std::vector<uint8_t> tx, rx;
  unsigned cursor = 0;
  void begin(int, int, int) {}
  void setTimeOut(int) {}
  void beginTransmission(uint8_t a) {
    address = a;
    tx.clear();
  }
  void write(uint8_t b) {
    tx.push_back(b);
  }
  int endTransmission(bool = true) {
    if(address==0x3C){
      if(!oled || (failOledData&&!tx.empty()&&tx[0]==0x40))return 4;
      if(!tx.empty()&&tx[0]==0x40)oledBytes+=tx.size()-1;
      if (tx.size() == 2 && tx[0] == 0x00 && (tx[1] == 0xAE || tx[1] == 0xAF)) {
        oledPowered = tx[1] == 0xAF;
      }
      return 0;
    }
    if (address == 0x70) {
      if (!mux || failMask) {
        return 4;
      }
      if (tx.size()) {
        mask = tx[0];
      }
      return 0;
    }
    if (address == 0x44) {
      for (int i = 0; i < 8; ++i) {
        if ((mask & (1 << i)) && sht[i]) {
          return 0;
        }
      }
      return 2;
    }
    if (address != 0x51 || !rtc) {
      return 2;
    }
    if (tx.size()) {
      pointer = tx[0];
    }
    if (tx.size() > 1) {
      if (pointer == failRegister) {
        return 4;
      }
      for (unsigned i = 1; i < tx.size(); ++i) {
        memory[uint8_t(pointer + i - 1)] = tx[i];
      }
    }
    return 0;
  }
  int requestFrom(uint8_t a, uint8_t count) {
    if (a == 0x44) {
      bool found = false;
      for (int i = 0; i < 8; ++i) {
        found = found || ((mask & (1 << i)) && sht[i]);
      }
      if (!found) {
        return 0;
      }
      auto crc = [](uint8_t a, uint8_t b) {
        uint8_t c = 0xff;
        for (uint8_t x : {a, b}) {
          c ^= x;
          for (int k = 0; k < 8; ++k) {
            c = (c & 128) ? (c << 1) ^ 0x31 : c << 1;
          }
        }
        return c;
      };
      rx = {0x66, 0x66, crc(0x66, 0x66), 0x80, 0x00, crc(0x80, 0x00)};
      if (badCrc) {
        rx[2] ^= 1;
      }
      cursor = 0;
      return 6;
    }
    if (a != 0x51 || !rtc) {
      return 0;
    }
    rx.clear();
    cursor = 0;
    for (int i = 0; i < count; ++i) {
      rx.push_back(memory[uint8_t(pointer + i)]);
    }
    return count;
  }
  uint8_t read() {
    return rx[cursor++];
  }
};
inline FakeWire Wire;

using TwoWire=FakeWire;
