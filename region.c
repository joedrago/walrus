#include "region.h"
#include "dyn.h"

#include <stdlib.h>
#include <stdio.h>

Region *RegionCreate(int type, const char *name)
{
    Region *region = calloc(1, sizeof(Region));
    region->type = type;
    dsCopy(&region->name, name);
    return region;
}

void RegionDestroy(Region *region)
{
    RegionClear(region);
    free(region);
}

void RegionClear(Region *region)
{
    dsDestroy(&region->name);
}
