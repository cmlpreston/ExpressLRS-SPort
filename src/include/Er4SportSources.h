#pragma once
#ifndef SPORT_CELLS_PHYSICAL_ID
#define SPORT_CELLS_PHYSICAL_ID 0xff
#endif
#ifndef SPORT_CELLS_APP_ID
#define SPORT_CELLS_APP_ID 0xffff
#endif
#ifndef SPORT_TEMP_PHYSICAL_ID
#define SPORT_TEMP_PHYSICAL_ID 0xff
#endif
#ifndef SPORT_TEMP_APP_ID
#define SPORT_TEMP_APP_ID 0xffff
#endif
#ifndef SPORT_GPS_PHYSICAL_ID
#define SPORT_GPS_PHYSICAL_ID 0xff
#endif
#ifndef SPORT_GPS_INSTANCE
#define SPORT_GPS_INSTANCE 0xff
#endif
// Sources remain unselected outside explicitly configured build environments.
constexpr bool er4SportCellsSelected = SPORT_CELLS_PHYSICAL_ID != 0xff && SPORT_CELLS_APP_ID != 0xffff;
