#ifndef _LEVEL_H
#define _LEVEL_H

void level_setup();

int level_get_distance();

struct LevelTankExt
{
    bool empty;
    int meas; // remaining water level in 'cm'
    int percent; // remaining water level in '%'
    int capacity; // remaining water capacity in 'L'
};

bool level_get_tank_ext(LevelTankExt &el);

#endif // _LEVEL_H
