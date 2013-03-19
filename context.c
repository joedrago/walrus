#include "context.h"
#include "bucket.h"
#include "identity.h"
#include "region.h"
#include "rule.h"
#include "cJSON.h"

#include <stdlib.h>
#include <stdio.h>

static const char *SS(const char *p, const char *def)
{
    if(p && *p)
    {
        return p;
    }
    return def;
}

Monitor *MonitorCreate()
{
    Monitor *m = (Monitor *)calloc(1, sizeof(Monitor));
    return m;
}

void MonitorDestroy(Monitor *m)
{
    free(m);
}

Context *ContextCreate()
{
    Context *context = calloc(1, sizeof(Context));

    Bucket *bucket = BucketCreate("Notepads");
    Identity *identity = IdentityCreate(NULL, "Notepad");
    Region *region = RegionCreate(REGIONTYPE_COORDS);
    Rule *rule = RuleCreate(0, -1, RULEFLAG_SPLIT|RULEFLAG_PERCENT, 0, RULESIDE_TOP, 100);
    region->r.left = 2485;
    region->r.top = 600;
    region->r.right = 3000;
    region->r.bottom = 1000;
    daPush(&bucket->identities, identity);
    daPush(&bucket->regions, region);
    daPush(&bucket->rules, rule);
    daPush(&context->buckets, bucket);

    context->mutex = CreateMutex(NULL, FALSE, NULL);

    return context;
}

void ContextDestroy(Context *context)
{
    ContextClear(context);
    CloseHandle(context->mutex);
    daDestroy(&context->monitors, MonitorDestroy);
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

static BOOL CALLBACK FoundMonitor(HMONITOR hMonitor, HDC hdcMonitor, LPRECT r, LPARAM dwData)
{
    Context *context = (Context *)dwData;
    Monitor *m = MonitorCreate();
    memcpy(&m->r, r, sizeof(RECT));
    daPush(&context->monitors, m);
    return TRUE;
}

void ContextUpdateMonitors(Context *context)
{
    daDestroy(&context->monitors, MonitorDestroy);
    EnumDisplayMonitors(NULL, NULL, FoundMonitor, (LPARAM)context);
}

void ContextUpdate(Context *context)
{
    WaitForSingleObject(context->mutex, INFINITE);
    ContextUpdateMonitors(context);
    ContextSearch(context);
    ContextLayout(context);
    ReleaseMutex(context->mutex);
}

void ContextGetConfig(Context *context, char **config)
{
    WaitForSingleObject(context->mutex, INFINITE);
    {
        int i;
        int bucketIndex;
        int identityIndex;
        int regionIndex;
        int ruleIndex;
        ContextUpdateMonitors(context);
        dsCopy(config, "{\"__monitors\":[");
        for(i = 0; i < daSize(&context->monitors); ++i)
        {
            Monitor *m = context->monitors[i];
            if(i)
                dsConcatf(config, ",");
            dsConcatf(config, "\"Monitor %d: %dx%d+%d+%d\"", i, m->r.right, m->r.bottom, m->r.left, m->r.top);
        }
        dsConcatf(config, "],\"buckets\": [");
        for(bucketIndex = 0; bucketIndex < daSize(&context->buckets); ++bucketIndex)
        {
            Bucket *bucket = context->buckets[bucketIndex];
            if(bucketIndex)
                dsConcatf(config, ",");
            dsConcatf(config, "{\"identities\":[");
            for(identityIndex = 0; identityIndex < daSize(&bucket->identities); ++identityIndex)
            {
                Identity *identity = bucket->identities[identityIndex];
                if(identityIndex)
                    dsConcatf(config, ",");
                dsConcatf(config, "{\"titleRegex\":\"%s\"",  SS(identity->titleRegex, "")); // TODO: escape
                dsConcatf(config, ",\"classRegex\":\"%s\"}", SS(identity->classRegex, "")); // TODO: escape
            }
            dsConcatf(config, "],\"regions\":[");
            for(regionIndex = 0; regionIndex < daSize(&bucket->regions); ++regionIndex)
            {
                Region *region = bucket->regions[regionIndex];
                if(regionIndex)
                    dsConcatf(config, ",");
                dsConcatf(config, "{\"type\":\"%s\"", RegionTypeString(region->type));
                dsConcatf(config, ",\"which\":%d", region->which);
                dsConcatf(config, ",\"r\":{\"left\":%d,\"top\":%d,\"right\":%d,\"bottom\":%d}}",
                    region->r.left, region->r.top, region->r.right, region->r.bottom
                );
            }
            dsConcatf(config, "],\"rules\":[");
            for(ruleIndex = 0; ruleIndex < daSize(&bucket->rules); ++ruleIndex)
            {
                Rule *rule = bucket->rules[ruleIndex];
                if(ruleIndex)
                    dsConcatf(config, ",");
                dsConcatf(config, "{\"from\":%d", rule->fromIndex);
                dsConcatf(config, ",\"to\":%d", rule->toIndex);
                dsConcatf(config, ",\"flags\":%d", rule->flags);
                dsConcatf(config, ",\"size\":%d", rule->size);
                dsConcatf(config, ",\"side\":%d", rule->side);
                dsConcatf(config, ",\"region\":%d}", rule->region);
            }
            dsConcatf(config, "]}");
        }
        dsConcatf(config, "]}");
    }
    ReleaseMutex(context->mutex);
}

static cJSON *jsonGetChild(cJSON *json, const char *name, int type)
{
    cJSON *child = cJSON_GetObjectItem(json, name);
    if(!child || (child->type != type))
        return NULL;
    return child;
}

static const char *jsonGetString(cJSON *json, const char *name)
{
    cJSON *child = jsonGetChild(json, name, cJSON_String);
    if(child)
    {
        return child->valuestring;
    }
    return "";
}

static int jsonGetInt(cJSON *json, const char *name)
{
    cJSON *child = jsonGetChild(json, name, cJSON_Number);
    if(child)
    {
        return child->valueint;
    }
    return 0;
}

void ContextSetConfig(Context *context, char *config)
{
    WaitForSingleObject(context->mutex, INFINITE);
    {
        cJSON *json = cJSON_Parse(config);
        if(json)
        {
            cJSON *jsonBuckets = jsonGetChild(json, "buckets", cJSON_Array);
            int apply = jsonGetInt(json, "apply");
            ContextClear(context);
            if(jsonBuckets)
            {
                cJSON *jsonBucket = jsonBuckets->child;
                for(; jsonBucket != NULL; jsonBucket = jsonBucket->next)
                {
                    cJSON *jsonIdentities = jsonGetChild(jsonBucket, "identities", cJSON_Array);
                    cJSON *jsonRegions    = jsonGetChild(jsonBucket, "regions",    cJSON_Array);
                    cJSON *jsonRules      = jsonGetChild(jsonBucket, "rules",      cJSON_Array);
                    Bucket *bucket = BucketCreate();
                    daPush(&context->buckets, bucket);
                    if(jsonIdentities)
                    {
                        cJSON *jsonIdentity = jsonIdentities->child;
                        for(; jsonIdentity != NULL; jsonIdentity = jsonIdentity->next)
                        {
                            Identity *identity = IdentityCreate(jsonGetString(jsonIdentity, "titleRegex"), jsonGetString(jsonIdentity, "classRegex"));
                            daPush(&bucket->identities, identity);
                        }
                    }
                    if(jsonRegions)
                    {
                        cJSON *jsonRegion = jsonRegions->child;
                        for(; jsonRegion != NULL; jsonRegion = jsonRegion->next)
                        {
                            cJSON *jsonR = jsonGetChild(jsonRegion, "r", cJSON_Object);
                            Region *region;
                            int type = RegionTypeFromString(jsonGetString(jsonRegion, "type"));
                            region = RegionCreate(type);
                            region->which = jsonGetInt(jsonR, "which");
                            if(jsonR)
                            {
                                region->r.left   = jsonGetInt(jsonR, "left");
                                region->r.top    = jsonGetInt(jsonR, "top");
                                region->r.right  = jsonGetInt(jsonR, "right");
                                region->r.bottom = jsonGetInt(jsonR, "bottom");
                            }
                            daPush(&bucket->regions, region);
                        }
                    }
                    if(jsonRules)
                    {
                        cJSON *jsonRule = jsonRules->child;
                        for(; jsonRule != NULL; jsonRule = jsonRule->next)
                        {
                            Rule *rule;
                            int from  = jsonGetInt(jsonRule, "from");
                            int to    = jsonGetInt(jsonRule, "to");
                            int which = jsonGetInt(jsonRule, "which");
                            int flags = jsonGetInt(jsonRule, "flags");
                            int side  = jsonGetInt(jsonRule, "side");
                            int size  = jsonGetInt(jsonRule, "size");
                            rule = RuleCreate(from, to, flags, which, side, size);
                            daPush(&bucket->rules, rule);
                        }
                    }
                }
            }
            cJSON_Delete(json);
            if(apply)
            {
                ContextUpdate(context);
            }
        }
    }
    ReleaseMutex(context->mutex);
}
