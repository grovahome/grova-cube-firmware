#pragma once
#include <stdint.h>
namespace rv3032 {
inline int unbcd(uint8_t value) {
  return (value & 15) > 9 || (value >> 4) > 9 ? -1 : (value >> 4) * 10 + (value & 15);
}
inline uint8_t bcd(int value) {
  return uint8_t((value / 10) * 16 + value % 10);
}
inline bool leap(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
inline int monthDays(int year, int month) {
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return month < 1 || month > 12 ? 0 : days[month - 1] + (month == 2 && leap(year));
}
// Gregorian UTC conversion; independent of host timezone and DST.
inline bool decode(const uint8_t* r, uint8_t status, uint8_t control, uint32_t& epoch) {
  if ((status & 3) || (control & 1)) {
    return false;  // PORF / VLF / STOP
  }
  int s = unbcd(r[0]), m = unbcd(r[1]), h = unbcd(r[2]), day = unbcd(r[4]);
  int mon = unbcd(r[5]), year = 2000 + unbcd(r[6]);
  if (s < 0 || s > 59 || m < 0 || m > 59 || h < 0 || h > 23 || r[3] > 6 || year < 2024 ||
      year > 2099 || mon < 1 || mon > 12 || day < 1 || day > monthDays(year, mon)) {
    return false;
  }
  uint32_t days = 0;
  for (int y = 1970; y < year; ++y) {
    days += leap(y) ? 366 : 365;
  }
  for (int mm = 1; mm < mon; ++mm) {
    days += monthDays(year, mm);
  }
  epoch = (days + day - 1) * 86400UL + h * 3600UL + m * 60UL + s;
  return true;
}
inline bool encode(uint32_t epoch, uint8_t* regs) {
  uint32_t days = epoch / 86400, rem = epoch % 86400;
  int year = 1970, month = 1;
  uint32_t weekday = (days + 4) % 7;
  while (days >= uint32_t(leap(year) ? 366 : 365)) {
    days -= leap(year) ? 366 : 365;
    ++year;
  }
  if (year < 2024 || year > 2099) {
    return false;
  }
  while (days >= uint32_t(monthDays(year, month))) {
    days -= monthDays(year, month);
    ++month;
  }
  regs[0] = bcd(rem % 60);
  regs[1] = bcd((rem / 60) % 60);
  regs[2] = bcd(rem / 3600);
  regs[3] = weekday;
  regs[4] = bcd(days + 1);
  regs[5] = bcd(month);
  regs[6] = bcd(year - 2000);
  return true;
}

}  // namespace rv3032
