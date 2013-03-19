#include "bucket.h"
#include "identity.h"
#include "region.h"
#include "rule.h"

#include <stdlib.h>
#include <stdio.h>

BucketWindow *BucketWindowCreate()
{
    BucketWindow *bw = calloc(1, sizeof(BucketWindow));
    return bw;
}

void BucketWindowDestroy(BucketWindow *bw)
{
    BucketWindowClear(bw);
    free(bw);
}

void BucketWindowClear(BucketWindow *bw)
{
}

Bucket *BucketCreate()
{
    Bucket *bucket = calloc(1, sizeof(Bucket));
    return bucket;
}

void BucketDestroy(Bucket *bucket)
{
    BucketClear(bucket);
    free(bucket);
}

void BucketClear(Bucket *bucket)
{
    daDestroy(&bucket->windows, BucketWindowDestroy);
    daDestroy(&bucket->identities, IdentityDestroy);
    daDestroy(&bucket->regions, RegionDestroy);
    daDestroy(&bucket->rules, RuleDestroy);
}

void BucketSearchBegin(Bucket *bucket)
{
    int i;
    for(i = 0; i < daSize(&bucket->windows); ++i)
    {
        bucket->windows[i]->found = 0;
    }
}

void BucketSearchEnd(Bucket *bucket)
{
    int i;
    for(i = 0; i < daSize(&bucket->windows); ++i)
    {
        if(!bucket->windows[i]->found)
        {
            char windowTitle[2048];
            windowTitle[0] = 0;
            GetWindowText(bucket->windows[i]->hwnd, windowTitle, 2048);
            printf("forgetting old one: %s\n", windowTitle);

            BucketWindowDestroy(bucket->windows[i]);
            bucket->windows[i] = NULL;
        }
    }
    daSquash(&bucket->windows);
}

int BucketAddWindow(Bucket *bucket, HWND hwnd, const char *windowTitle, const char *windowClass)
{
    int keep = 0;
    int i;

    for(i = 0; i < daSize(&bucket->identities); ++i)
    {
        if(IdentityMatches(bucket->identities[i], windowTitle, windowClass))
        {
            keep = 1;
            break;
        }
    }

    if(keep)
    {
        BucketWindow *bw = NULL;
        for(i = 0; i < daSize(&bucket->windows); ++i)
        {
            if(bucket->windows[i]->hwnd == hwnd)
            {
                printf("keeping previous: %s\n", windowTitle);
                bw = bucket->windows[i];
                break;
            }
        }
        if(!bw)
        {
            bw = BucketWindowCreate();
            bw->hwnd = hwnd;
            printf("keeping new one: %s\n", windowTitle);
            daPush(&bucket->windows, bw);
        }
        bw->found = 1;
    }
    return keep;
}

static Region *BucketFindRegion(Bucket *bucket, int index)
{
    if((index >= 0) && (index < daSize(&bucket->regions)))
    {
        return bucket->regions[index];
    }
    return NULL;
}

void BucketLayout(Bucket * bucket)
{
    int regionIndex;
    int ruleIndex;

    if(!daSize(&bucket->windows))
    {
        printf("no matching windows, nothing to do\n");
        return;
    }

    for (regionIndex = 0; regionIndex < daSize(&bucket->regions); ++regionIndex)
    {
        Region * region = bucket->regions[regionIndex];
        region->leftover = region->r;
        // TODO: add other region types like MONITOR
    }

    for (ruleIndex = 0; ruleIndex < daSize(&bucket->rules); ++ruleIndex)
    {
        Rule *rule = bucket->rules[ruleIndex];
        Region *region = BucketFindRegion(bucket, rule->region);
        int from = rule->fromIndex;
        int to = rule->toIndex;
        int windowIndex;
        int windowCount;
        int windowPortion;
        RECT slice = {0};

        if(region == NULL)
        {
            printf("failed to find region '%s', bailing out\n", rule->region);
            return;
        }

        if(to == -1)
        {
            to = daSize(&bucket->windows) - 1;
        }

        // TODO: sanity check from/to
        windowCount = (to - from) + 1; // inclusive!
        if(windowCount < 1)
        {
            printf("window count < 1, bailing out [%d]\n", windowCount);
            return;
        }

        printf("running rule %d on region '%d' [%d windows]\n", ruleIndex, rule->region, windowCount);

        slice = RuleSlice(rule, region);

        if(rule->flags & RULEFLAG_SPLIT)
        {
            if(rule->flags & RULEFLAG_HORIZONTAL)
            {
                windowPortion = (slice.bottom - slice.top) / windowCount;
                slice.bottom -= (windowPortion * (windowCount - 1));
            }
            else
            {
                windowPortion = (slice.right - slice.left) / windowCount;
                slice.right -= (windowPortion * (windowCount - 1));
            }
        }

        for(windowIndex = 0; windowIndex < daSize(&bucket->windows); ++windowIndex)
        {
            BucketWindow *bw = bucket->windows[windowIndex];

            if(rule->flags & RULEFLAG_MAXIMIZE)
            {
                // TODO: move to the proper monitor
                ShowWindow(bw->hwnd, SW_MAXIMIZE);
            }
            else if(rule->flags & RULEFLAG_MINIMIZE)
            {
                // TODO: move to the proper monitor
                ShowWindow(bw->hwnd, SW_MINIMIZE);
            }
            else
            {
                printf("setting window %d to [%d, %d, %d, %d]\n",
                        windowIndex,
                        slice.left,
                        slice.top,
                        slice.right,
                        slice.bottom
                      );

                ShowWindow(bw->hwnd, SW_RESTORE); // necessary?
                SetWindowPos(bw->hwnd, 0,
                        slice.left, slice.top,
                        (slice.right - slice.left), (slice.bottom - slice.top),
                        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW
                        );

                if(rule->flags & RULEFLAG_SPLIT)
                {
                    if(rule->flags & RULEFLAG_HORIZONTAL)
                    {
                        slice.top += windowPortion;
                        slice.bottom += windowPortion;
                    }
                    else
                    {
                        slice.left += windowPortion;
                        slice.right += windowPortion;
                    }
                }
            }
        }
    }
}
