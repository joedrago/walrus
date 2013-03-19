#ifndef REGION_H
#define REGION_H

#include <windows.h>

enum // RegionType
{
    REGIONTYPE_ALL = 0,
    REGIONTYPE_MONITOR,
    REGIONTYPE_COORDS,

    REGIONTYPE_COUNT
};

typedef struct Region
{
    int type;
    int which; // monitor index for type MONITOR
    RECT r;

    RECT leftover; // used during layout
} Region;

Region *RegionCreate(int type);
void RegionDestroy(Region *region);
void RegionClear(Region *region);

const char *RegionTypeString(int type);
int RegionTypeFromString(const char *s);

#endif
