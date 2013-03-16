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
    char *name;
    int type;
    int which; // for REGIONTYPE_MONITOR
    RECT r;

    RECT leftover; // used during layout
} Region;

Region *RegionCreate(int type, const char *name);
void RegionDestroy(Region *region);
void RegionClear(Region *region);

#endif
