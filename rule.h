#ifndef RULE_H
#define RULE_H

#include <windows.h>

struct Region;

enum // RuleType
{
    RULEFLAG_SLICE          = (1 << 0),
    RULEFLAG_HORIZONTAL     = (1 << 1),
    RULEFLAG_VERTICAL       = (1 << 2),
    RULEFLAG_PIXELS         = (1 << 3),
    RULEFLAG_PERCENT        = (1 << 4),

    RULETYPE_IGNORE         = 0,
    RULETYPE_MAXIMIZE       = (1 << 5),
    RULETYPE_MINIMIZE       = (1 << 6),
    RULETYPE_HSLICE_PIXELS  = RULEFLAG_SLICE | RULEFLAG_HORIZONTAL | RULEFLAG_PIXELS,
    RULETYPE_HSLICE_PERCENT = RULEFLAG_SLICE | RULEFLAG_HORIZONTAL | RULEFLAG_PERCENT,
    RULETYPE_VSLICE_PIXELS  = RULEFLAG_SLICE | RULEFLAG_VERTICAL   | RULEFLAG_PIXELS,
    RULETYPE_VSLICE_PERCENT = RULEFLAG_SLICE | RULEFLAG_VERTICAL   | RULEFLAG_PERCENT
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
    char *region;
    int fromIndex;
    int toIndex;    // -1 is 'the rest'
    int type;
    int side;
    int size; // for SLICE*
} Rule;

Rule *RuleCreate(int from, int to, int type, const char *region, int side, int size);
void RuleDestroy(Rule *rule);
void RuleClear(Rule *rule);

RECT RuleSlice(Rule *rule, struct Region *region);

#endif
