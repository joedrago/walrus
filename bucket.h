#ifndef BUCKET_H
#define BUCKET_H

#include "dyn.h"
#include <windows.h>

struct Identity;
struct Region;
struct Rule;

typedef struct BucketWindow
{
    HWND hwnd;
    int found;
} BucketWindow;

BucketWindow *BucketWindowCreate();
void BucketWindowDestroy(BucketWindow *bw);
void BucketWindowClear(BucketWindow *bw);

typedef struct Bucket
{
    char *name;
    struct Identity **identities;
    BucketWindow **windows;
    struct Region **regions;
    struct Rule **rules;
} Bucket;

Bucket *BucketCreate(const char *name);
void BucketDestroy(Bucket *bucket);
void BucketClear(Bucket *bucket);

void BucketSearchBegin(Bucket *bucket); // called before an EnumWindows
void BucketSearchEnd(Bucket *bucket);   // called after an EnumWindows, removes all "not found"

int BucketAddWindow(Bucket *bucket, HWND hwnd, const char *windowTitle, const char *windowClass); // returns 1 if it kept it / had it

void BucketLayout(Bucket *bucket);

#endif
