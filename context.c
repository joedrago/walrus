#include "context.h"
#include "bucket.h"
#include "identity.h"
#include "region.h"
#include "rule.h"

#include <stdlib.h>
#include <stdio.h>

Context *ContextCreate()
{
    Context *context = calloc(1, sizeof(Context));
    Bucket *bucket = BucketCreate("Notepads");
    Identity *identity = IdentityCreate(NULL, "Notepad");
    Region *region = RegionCreate(REGIONTYPE_COORDS, "main");
    Rule *rule = RuleCreate(0, -1, RULETYPE_VSLICE_PERCENT, "main", RULESIDE_TOP, 100);
    region->r.left = 1685;
    region->r.top = 5;
    region->r.right = 3000;
    region->r.bottom = 600;
    daPush(&bucket->identities, identity);
    daPush(&bucket->regions, region);
    daPush(&bucket->rules, rule);
    daPush(&context->buckets, bucket);
    return context;
}

void ContextDestroy(Context *context)
{
    ContextClear(context);
    free(context);
}

void ContextClear(Context *context)
{
    daDestroy(&context->buckets, BucketDestroy);
}

static BOOL CALLBACK FoundWindow(HWND hwnd, LPARAM lParam)
{
    int i;
    Context *context = (Context *)lParam;
    char windowTitle[2048];
    char windowClass[2048];
    windowTitle[0] = 0;
    windowClass[0] = 0;
    GetWindowText(hwnd, windowTitle, 2048);
    GetClassName (hwnd, windowClass, 2048);

    for(i = 0; i < daSize(&context->buckets); ++i)
    {
        if(BucketAddWindow(context->buckets[i], hwnd, windowTitle, windowClass))
        {
            break;
        }
    }
    return TRUE;
}

void ContextSearch(Context *context)
{
    int i;
    for(i = 0; i < daSize(&context->buckets); ++i)
    {
        BucketSearchBegin(context->buckets[i]);
    }
    EnumWindows(FoundWindow, (LPARAM)context);
    for(i = 0; i < daSize(&context->buckets); ++i)
    {
        BucketSearchEnd(context->buckets[i]);
    }
}

void ContextLayout(Context *context)
{
    int i;
    for(i = 0; i < daSize(&context->buckets); ++i)
    {
        BucketLayout(context->buckets[i]);
    }
}

void ContextUpdate(Context *context)
{
    ContextSearch(context);
    ContextLayout(context);
}
