#pragma once
#include <cstdint>
enum State { disconnected, connected, wifiUpdate, serialUpdate, radioFailed };
extern State connectionState;
