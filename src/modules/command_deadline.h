#pragma once
#include <stdint.h>
#include <string.h>

// Zero means an older/local client without a deadline. A stamped command
// requires a usable clock and expires at the boundary, not one second later.
inline bool commandDeadlineAllows(uint32_t expiry, bool synced, uint32_t now) {
  return expiry == 0 || (synced && now < expiry);
}

class RecentCommandIds {
  char ids[16][48] = {};
  unsigned next = 0;
public:
  bool accept(const char* id) {
    if (!id || !*id) return true; // Legacy clients without command identity.
    for (const auto& entry : ids) if (strcmp(entry, id) == 0) return false;
    strncpy(ids[next], id, sizeof(ids[next]) - 1);
    ids[next][sizeof(ids[next]) - 1] = 0;
    next = (next + 1) % 16;
    return true;
  }
};
