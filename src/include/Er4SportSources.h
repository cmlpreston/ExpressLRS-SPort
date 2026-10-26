#pragma once
#include "SportSources.h"
// Sources remain unselected outside explicitly configured build environments.
constexpr bool er4SportCellsSelected = SPORT_CELLS_PHYSICAL_ID != 0xff && SPORT_CELLS_APP_ID != 0xffff;
