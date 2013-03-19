#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"
#include "dyn.h"

#include "context.h"
#include "bucket.h"
#include "identity.h"
#include "region.h"
#include "rule.h"

#include "mongoose.h"

#define LISTEN_PORT 8099
#define POSTDATA_READ_SIZE 512

int picking = 0;
static Context *sContext = NULL;
static sCurrentBucket = -1;
static sCurrentIdentity = -1;

static void pick(HWND hwnd)
{
    //picking = 1;
    ShellExecute(NULL, NULL, "http://localhost:8099/", NULL, NULL, SW_SHOW);
}

static void debug()
{
    ContextUpdate(sContext);
}

#define DISABLE(ID) EnableWindow(GetDlgItem(hDlg, ID), 0)
#define DISABLEC(ID) { HWND HHH = GetDlgItem(hDlg, ID);  EnableWindow(HHH, 0); SetWindowText(HHH, ""); }
#define ENABLE(ID) EnableWindow(GetDlgItem(hDlg, ID), 1)

static void ctrlDisableIdentity(HWND hDlg)
{
    DISABLE(IDC_IDENTITIES);
    DISABLE(IDC_IDENTITY_NEW);
    DISABLE(IDC_IDENTITY_DELETE);
    DISABLE(IDC_IDENTITY_TITLE);
    DISABLE(IDC_IDENTITY_CLASS);
    DISABLE(IDC_IDENTITY_STEAL);
}

static void ctrlDisableRegion(HWND hDlg)
{
    DISABLE(IDC_REGIONS);
    DISABLE(IDC_REGION_NEW);
    DISABLE(IDC_REGION_DELETE);
    DISABLE(IDC_REGION_NAME);
    DISABLE(IDC_REGION_ALL);
    DISABLE(IDC_REGION_MONITOR);
    DISABLE(IDC_REGION_WHICH);
    DISABLE(IDC_REGION_COORDS);
    DISABLE(IDC_REGION_TOP);
    DISABLE(IDC_REGION_LEFT);
    DISABLE(IDC_REGION_RIGHT);
    DISABLE(IDC_REGION_BOTTOM);
    DISABLE(IDC_REGION_STEAL);
}

static void ctrlDisableRule(HWND hDlg)
{
    DISABLE(IDC_RULES);
    DISABLE(IDC_RULE_NEW);
    DISABLE(IDC_RULE_DELETE);
    DISABLE(IDC_RULE_FROM);
    DISABLE(IDC_RULE_TO);
    DISABLE(IDC_RULE_REST);
    DISABLE(IDC_RULE_MAXIMIZE);
    DISABLE(IDC_RULE_MINIMIZE);
    DISABLE(IDC_RULE_OVERLAP);
    DISABLE(IDC_RULE_SPLIT);
    DISABLE(IDC_RULE_HORIZONTAL);
    DISABLE(IDC_RULE_VERTICAL);
    DISABLE(IDC_RULE_SIZE);
    DISABLE(IDC_RULE_PERCENT);
    DISABLE(IDC_RULE_SIDE);
}

static void ctrlDisableBucket(HWND hDlg)
{
    DISABLE(IDC_BUCKET_NAME);
}

static void ctrlDisableAll(HWND hDlg)
{
    ctrlDisableIdentity(hDlg);
    ctrlDisableRegion(hDlg);
    ctrlDisableRule(hDlg);
    ctrlDisableBucket(hDlg);
    sCurrentBucket = -1;
}

static void ctrlPushBuckets(HWND hDlg)
{
    int i;
    HWND list = GetDlgItem(hDlg, IDC_BUCKETS);
    ctrlDisableAll(hDlg);
    ListBox_ResetContent(list);
    for(i = 0; i < daSize(&sContext->buckets); ++i)
    {
        Bucket *bucket = sContext->buckets[i];
        //ListBox_AddString(list, bucket->name);
    }
}

static int ctrlStringFromID(HWND hDlg, int id, char **s)
{
    char temp[2048];
    if(GetWindowText(GetDlgItem(hDlg, id), temp, 2048))
    {
        dsCopy(s, temp);
        return 1;
    }
    return 0;
}

static void ctrlPullIdentity(HWND hDlg)
{
    Identity *identity;
    if(sCurrentBucket == -1)
        return;
    if(sCurrentIdentity == -1)
        return;

    identity = sContext->buckets[sCurrentBucket]->identities[sCurrentIdentity];
    ctrlStringFromID(hDlg, IDC_IDENTITY_TITLE, &identity->titleRegex);
    ctrlStringFromID(hDlg, IDC_IDENTITY_CLASS, &identity->classRegex);
    DISABLEC(IDC_IDENTITY_TITLE);
    DISABLEC(IDC_IDENTITY_CLASS);
    DISABLE(IDC_IDENTITY_STEAL);

    sCurrentIdentity = -1;
}

static void ctrlPushIdentity(HWND hDlg)
{
//    int i;
    int current;
//    HWND list;
    Identity *identity;
    if(sCurrentBucket == -1)
        return;

    ctrlPullIdentity(hDlg);
    current = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_IDENTITIES));
    if((current < 0) || (current >= daSize(&sContext->buckets[sCurrentBucket]->identities)))
    {
        ctrlDisableIdentity(hDlg);
        return;
    }

    identity = sContext->buckets[sCurrentBucket]->identities[current];

    ENABLE(IDC_IDENTITY_TITLE);
    SetWindowText(GetDlgItem(hDlg, IDC_IDENTITY_TITLE), identity->titleRegex);
    ENABLE(IDC_IDENTITY_CLASS);
    SetWindowText(GetDlgItem(hDlg, IDC_IDENTITY_CLASS), identity->classRegex);
    ENABLE(IDC_IDENTITY_STEAL);

    sCurrentIdentity = current;
}

static void ctrlPushRegion(HWND hDlg)
{
}

static void ctrlPushRule(HWND hDlg)
{
}

// safe string
static const char *SS(const char *p, const char *def)
{
    if(p && *p)
    {
        return p;
    }
    return def;
}

static void ctrlPullBucket(HWND hDlg)
{
    if(sCurrentBucket == -1)
        return;

    ctrlPullIdentity(hDlg);

    sCurrentBucket = -1;
}

static void ctrlPushBucket(HWND hDlg)
{
    int i;
    HWND list;
    Bucket *bucket;
    int current;

    ctrlPullBucket(hDlg);

    current = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_BUCKETS));
    if((current < 0) || (current >= daSize(&sContext->buckets)))
    {
        ctrlDisableAll(hDlg);
        return;
    }

    bucket = sContext->buckets[current];

    ENABLE(IDC_BUCKET_NAME);
    //SetWindowText(GetDlgItem(hDlg, IDC_BUCKET_NAME), bucket->name);

    ENABLE(IDC_IDENTITIES);
    list = GetDlgItem(hDlg, IDC_IDENTITIES);
    ListBox_ResetContent(list);
    for(i = 0; i < daSize(&bucket->identities); ++i)
    {
        Identity *identity = bucket->identities[i];
        char temp[2048]; // lame
        _snprintf(temp, 2048, "%s / %s", SS(identity->titleRegex, "--"), SS(identity->classRegex, "--"));
        temp[2047] = 0;
        ListBox_AddString(list, temp);
    }
    ENABLE(IDC_IDENTITY_NEW);
    ENABLE(IDC_IDENTITY_DELETE);

    ENABLE(IDC_REGIONS);
    list = GetDlgItem(hDlg, IDC_REGIONS);
    ListBox_ResetContent(list);
    for(i = 0; i < daSize(&bucket->regions); ++i)
    {
        Region *region = bucket->regions[i];
        //ListBox_AddString(list, region->name);
    }
    ENABLE(IDC_REGION_NEW);
    ENABLE(IDC_REGION_DELETE);

    /*
    ENABLE(IDC_REGION_NAME);
    ENABLE(IDC_REGION_ALL);
    ENABLE(IDC_REGION_MONITOR);
    ENABLE(IDC_REGION_WHICH);
    ENABLE(IDC_REGION_COORDS);
    ENABLE(IDC_REGION_TOP);
    ENABLE(IDC_REGION_BOTTOM);
    ENABLE(IDC_REGION_LEFT);
    ENABLE(IDC_REGION_RIGHT);
    ENABLE(IDC_REGION_STEAL);
    */

    ENABLE(IDC_RULES);
    list = GetDlgItem(hDlg, IDC_RULES);
    ListBox_ResetContent(list);
    for(i = 0; i < daSize(&bucket->rules); ++i)
    {
        Rule *rule = bucket->rules[i];
        ListBox_AddString(list, "Rule");
    }
    ENABLE(IDC_RULE_NEW);
    ENABLE(IDC_RULE_DELETE);

    /*
    ENABLE(IDC_RULE_FROM);
    ENABLE(IDC_RULE_TO);
    ENABLE(IDC_RULE_REST);
    ENABLE(IDC_RULE_MAXIMIZE);
    ENABLE(IDC_RULE_MINIMIZE);
    ENABLE(IDC_RULE_OVERLAP);
    ENABLE(IDC_RULE_SPLIT);
    ENABLE(IDC_RULE_HORIZONTAL);
    ENABLE(IDC_RULE_VERTICAL);
    ENABLE(IDC_RULE_SIZE);
    ENABLE(IDC_RULE_PERCENT);
    ENABLE(IDC_RULE_SIDE);
    */

    sCurrentBucket = current;
}

static void ctrlNewBucket(HWND hDlg)
{
    Bucket *bucket = BucketCreate("New");
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
    daPush(&sContext->buckets, bucket);

    ctrlPushBuckets(hDlg);
}

static void ctrlNewIdentity(HWND hDlg)
{
    if(sCurrentBucket == -1)
        return;

    ctrlPullIdentity(hDlg);
}

INT_PTR CALLBACK WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_ACTIVATE:
        {
            if(wParam == WA_INACTIVE)
            {
                if(picking)
                {
                    printf("you clicked on something else!\n");
                    SetTimer(hDlg, 5, 500, NULL);
                }
            }
        }
        break;

        case WM_TIMER:
        {
            if(picking)
            {
                HWND foreground = GetForegroundWindow();
                if(foreground)
                {
                    char windowTitle[2048];
                    windowTitle[0] = 0;
                    GetClassName(foreground, windowTitle, 2048);
                    printf("you clicked on: %s\n", windowTitle);
                }
                picking = 0;
            }
            KillTimer(hDlg, 5);
        }
        break;

        case WM_INITDIALOG:
            {
                ComboBox_AddString(GetDlgItem(hDlg, IDC_RULE_PERCENT), "Percent");
                ComboBox_AddString(GetDlgItem(hDlg, IDC_RULE_PERCENT), "Pixels");
                ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_RULE_PERCENT), 0);

                ComboBox_AddString(GetDlgItem(hDlg, IDC_RULE_SIDE), "Top");
                ComboBox_AddString(GetDlgItem(hDlg, IDC_RULE_SIDE), "Bottom");
                ComboBox_AddString(GetDlgItem(hDlg, IDC_RULE_SIDE), "Left");
                ComboBox_AddString(GetDlgItem(hDlg, IDC_RULE_SIDE), "Right");
                ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_RULE_SIDE), 0);

                ctrlPushBuckets(hDlg);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            else if (LOWORD(wParam) == IDC_PICK)
            {
                pick(hDlg);
            }
            else if (LOWORD(wParam) == IDC_DEBUG)
            {
                debug();
            }

#define CLICKED(ID, func) \
    else if(LOWORD(wParam) == ID) func(hDlg)
#define UPDATE(ID, NOTIFICATION, func) \
    else if((LOWORD(wParam) == ID) && (HIWORD(wParam) == NOTIFICATION)) func(hDlg)

            CLICKED(IDC_BUCKET_NEW, ctrlNewBucket);
            CLICKED(IDC_IDENTITY_NEW, ctrlNewIdentity);
            UPDATE(IDC_BUCKETS, LBN_SELCHANGE, ctrlPushBucket);
            UPDATE(IDC_IDENTITIES, LBN_SELCHANGE, ctrlPushIdentity);
            UPDATE(IDC_REGIONS, LBN_SELCHANGE, ctrlPushRegion);
            UPDATE(IDC_RULES, LBN_SELCHANGE, ctrlPushRule);

            break;
    }
    return (INT_PTR)FALSE;
}

static int begin_request_handler(struct mg_connection *conn)
{
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    char *content = NULL;
    char *postdata = NULL;
    const char *contentType = "text/plain";
    int ok = 1;

    printf("URI: %s\n", request_info->uri);

    if(!strcmp(request_info->uri, "/"))
    {
        FILE *f = fopen("config.html", "rb");
        if(f)
        {
            int s;
            fseek(f, 0, SEEK_END);
            s = (int)ftell(f);
            fseek(f, 0, SEEK_SET);

            if(s > 0)
            {
                dsSetLength(&content, s);
                fread(content, 1, s, f);
                contentType = "text/html";
            }
            else
            {
                ok = 0;
                dsPrintf(&content, "failed to read file");
            }
            fclose(f);
        }
        else
        {
            ok = 0;
            dsPrintf(&content, "failed to open file");
        }
    }
    else if(!strcmp(request_info->uri, "/config"))
    {
        ContextGetConfig(sContext, &content);
        contentType = "application/json";
    }
    else if(!strcmp(request_info->uri, "/set"))
    {
        int amtRead;
        int offset = 0;
        dsSetCapacity(&postdata, POSTDATA_READ_SIZE);
        postdata[0] = 0;
        while(amtRead = mg_read(conn, postdata + offset, POSTDATA_READ_SIZE))
        {
            offset += amtRead;
            dsCalcLength(&postdata);
            dsSetCapacity(&postdata, dsLength(&postdata) + POSTDATA_READ_SIZE);
        }
        printf("got: '%s'\n", postdata);
        ContextSetConfig(sContext, postdata);
        dsPrintf(&content, "OK");
    }
    else
    {
        ok = 0;
        dsPrintf(&content, "Hurr durr!");
    }

    if(ok)
    {
        mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n"
                "\r\n"
                "%s",
                contentType,
                (int)dsLength(&content), content);
    }
    else
    {
        mg_printf(conn,
                "HTTP/1.1 404 File Not Found\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n"
                "\r\n"
                "%s",
                contentType,
                (int)dsLength(&content), content);
    }

    dsDestroy(&content);
    dsDestroy(&postdata);

    // Returning non-zero tells mongoose that our function has replied to
    // the client, and mongoose should not send client any more data.
    return 1;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  struct mg_context *ctx;
  struct mg_callbacks callbacks;

  char listenport[64] = {0};

  const char *options[] = {"listening_ports", listenport, NULL};

#if _DEBUG
    if(AllocConsole())
    {
        freopen("CONOUT$", "wt", stdout);
        SetConsoleTitle("Walrus Debug Output");
    }
#endif

    sprintf(listenport, "127.0.0.1:%d", LISTEN_PORT);

    sContext = ContextCreate();

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.begin_request = begin_request_handler;

    ctx = mg_start(&callbacks, NULL, options);

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_WALRUS_DIALOG), NULL, WndProc);

    if(ctx)
    {
        mg_stop(ctx);
    }

    ContextDestroy(sContext);
    return 0;
}
