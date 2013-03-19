#include "rule.h"
#include "region.h"
#include "dyn.h"

#include <stdlib.h>
#include <stdio.h>

Rule *RuleCreate(int from, int to, int flags, int region, int side, int size)
{
    Rule *rule = calloc(1, sizeof(Rule));
    rule->fromIndex = from;
    rule->toIndex = to;
    rule->flags = flags;
    rule->side = side;
    rule->size = size;
    rule->region = region;
    return rule;
}

void RuleDestroy(Rule *rule)
{
    RuleClear(rule);
    free(rule);
}

void RuleClear(Rule *rule)
{
}

RECT RuleSlice(Rule * rule, Region * region)
{
    RECT slice;
    int wanted = rule->size;
    int remaining;
    int available;
    memset(&slice, 0, sizeof(RECT));

    switch (rule->side)
    {
        case RULESIDE_TOP:
        case RULESIDE_BOTTOM:
        {
            available = region->leftover.bottom - region->leftover.top;
        }
        break;
        case RULESIDE_LEFT:
        case RULESIDE_RIGHT:
        {
            available = region->leftover.right - region->leftover.left;
        }
        break;
    }

    if(rule->flags & RULEFLAG_PERCENT)
    {
        wanted = (available * rule->size) / 100;
    }
    remaining = available - wanted;

    slice = region->leftover;
    switch(rule->side)
    {
        case RULESIDE_TOP:
        {
            region->leftover.top += wanted;
            slice.bottom -= remaining;
        }
        break;

        case RULESIDE_BOTTOM:
        {
            region->leftover.bottom -= wanted;
            slice.top += remaining;
        }
        break;

        case RULESIDE_LEFT:
        {
            region->leftover.left += wanted;
            slice.right -= remaining;
        }
        break;

        case RULESIDE_RIGHT:
        {
            region->leftover.right -= wanted;
            slice.left += remaining;
        }
        break;
    }
    return slice;
}
