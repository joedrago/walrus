#ifndef CONTEXT_H
#define CONTEXT_H

#include <windows.h>

struct Bucket;

typedef struct Monitor
{
    RECT r;
} Monitor;

Monitor *MonitorCreate();
void MonitorDestroy(Monitor *m);

typedef struct Context
{
    struct Bucket **buckets;
    Monitor **monitors;
    HANDLE mutex;
} Context;

Context *ContextCreate();
void ContextDestroy(Context *context);
void ContextClear(Context *context);
void ContextUpdate(Context *context);
void ContextGetConfig(Context *context, char **config);
void ContextSetConfig(Context *context, char *config);

#endif
