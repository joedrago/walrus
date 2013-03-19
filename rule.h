#ifndef RULE_H
#define RULE_H

#include <windows.h>

struct Region;

enum // Rule Flags
{
    RULEFLAG_SPLIT          = (1 << 0), // overlap or split?
    RULEFLAG_HORIZONTAL     = (1 << 1), // horizontal or vertical?
    RULEFLAG_PERCENT        = (1 << 2), // pixels or percent?
    RULEFLAG_MAXIMIZE       = (1 << 3), // trumps MINIMIZE
    RULEFLAG_MINIMIZE       = (1 << 4)
};

enum // RuleSide
{
    RULESIDE_TOP = 0,
    RULESIDE_BOTTOM,
    RULESIDE_LEFT,
    RULESIDE_RIGHT,

    RULESIDE_COUNT
};

typedef struct Rule
{
    int fromIndex;
    int toIndex;    // -1 is 'the rest'
    int flags;
    int side;
    int size; // for SLICE*
    int region;
} Rule;

Rule *RuleCreate(int from, int to, int flags, int region, int side, int size);
void RuleDestroy(Rule *rule);
void RuleClear(Rule *rule);

RECT RuleSlice(Rule *rule, struct Region *region);

#endif
