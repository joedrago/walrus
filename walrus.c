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
            break;
    }
    return (INT_PTR)FALSE;
}

static int serve(const char *filename, char **content)
{
    int ok = 1;
    FILE *f = fopen(filename, "rb");
    if(f)
    {
        int s;
        fseek(f, 0, SEEK_END);
        s = (int)ftell(f);
        fseek(f, 0, SEEK_SET);

        if(s > 0)
        {
            dsSetLength(content, s);
            fread(*content, 1, s, f);
        }
        else
        {
            ok = 0;
            dsPrintf(content, "failed to read file");
        }
        fclose(f);
    }
    else
    {
        ok = 0;
        dsPrintf(content, "failed to open file");
    }
    return ok;
}

static void escape(char *str, char *output)
{
    char *src = str;
    char *dst = output;
    for(; *src; ++src)
    {
        switch(*src)
        {
            case '\\':
            case '"':
            {
                *dst = '\\';
                ++dst;
            }
        }

        *dst = *src;
        ++dst;
    }
    *dst = 0;
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
        ok = serve("config.html", &content);
        contentType = "text/html";
    }
    else if(!strcmp(request_info->uri, "/black.png"))
    {
        ok = serve("black.png", &content);
        contentType = "image/png";
    }
    else if(!strcmp(request_info->uri, "/red.png"))
    {
        ok = serve("red.png", &content);
        contentType = "image/png";
    }
    else if(!strcmp(request_info->uri, "/green.png"))
    {
        ok = serve("green.png", &content);
        contentType = "image/png";
    }
    else if(!strcmp(request_info->uri, "/selectarrow.jpg"))
    {
        ok = serve("selectarrow.jpg", &content);
        contentType = "image/jpeg";
    }
    else if(!strcmp(request_info->uri, "/config"))
    {
        ContextGetConfig(sContext, &content);
        contentType = "application/json";
    }
    else if(!strcmp(request_info->uri, "/steal"))
    {
        RECT r = {0};
        HWND h = GetForegroundWindow();
        char windowTitle[2048];
        char windowClass[2048];
        char windowTitleEsc[2048];
        char windowClassEsc[2048];
        char *temp = NULL;
        windowTitle[0] = 0;
        windowClass[0] = 0;
        windowTitleEsc[0] = 0;
        windowClassEsc[0] = 0;
        if(h)
        {
            GetWindowText(h, windowTitle, 2048);
            GetClassName (h, windowClass, 2048);
            escape(windowTitle, windowTitleEsc);
            escape(windowClass, windowClassEsc);
            GetWindowRect(h, &r);
        }
        dsPrintf(&content, "{\"left\":%d,\"top\":%d,\"right\":%d,\"bottom\":%d,\"title\":\"%s\",\"class\":\"%s\"}",
            r.left, r.top, r.right, r.bottom, windowTitleEsc, windowClassEsc
        );
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
                "\r\n",
                contentType,
                (int)dsLength(&content));
        mg_write(conn, content, (int)dsLength(&content));
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
