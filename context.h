#ifndef CONTEXT_H
#define CONTEXT_H

struct Bucket;

typedef struct Context
{
    struct Bucket **buckets;
} Context;

Context *ContextCreate();
void ContextDestroy(Context *context);
void ContextClear(Context *context);
void ContextUpdate(Context *context);

#endif
