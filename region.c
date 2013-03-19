#include "region.h"
#include "dyn.h"

#include <stdlib.h>
#include <stdio.h>

Region *RegionCreate(int type)
{
    Region *region = calloc(1, sizeof(Region));
    region->type = type;
    return region;
}

void RegionDestroy(Region *region)
{
    RegionClear(region);
    free(region);
}

void RegionClear(Region *region)
{
}

const char *RegionTypeString(int type)
{
    switch(type)
    {
        case REGIONTYPE_ALL:     return "all";
        case REGIONTYPE_MONITOR: return "monitor";
        case REGIONTYPE_COORDS:  return "coords";
    }
    return "";
}

int RegionTypeFromString(const char *s)
{
    if(!strcmp(s, "monitor"))
        return REGIONTYPE_MONITOR;
    if(!strcmp(s, "coords"))
        return REGIONTYPE_COORDS;
    return REGIONTYPE_ALL;
}
