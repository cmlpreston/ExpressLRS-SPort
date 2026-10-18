#pragma once
#include <cstdint>
enum State { connected, tentative, awaitingModelId, disconnected, MODE_STATES,
    noCrossfire, bleJoystick, NO_CONFIG_SAVE_STATES, wifiUpdate, serialUpdate,
    FAILURE_STATES, radioFailed, hardwareUndefined };
extern State connectionState;
