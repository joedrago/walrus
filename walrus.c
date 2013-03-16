#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"
#include "dyn.h"

#include "context.h"
#include "bucket.h"

int picking = 0;
static Context *sContext = NULL;

static void pick(HWND hwnd)
{
    picking = 1;
}

static void debug()
{
    ContextUpdate(sContext);
}

INT_PTR CALLBACK WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
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
            if (LOWORD(wParam) == IDC_PICK)
            {
                pick(hDlg);
            }
            else if (LOWORD(wParam) == IDC_DEBUG)
            {
                debug();
            }
            else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#if _DEBUG
    AllocConsole();
    SetConsoleTitle("Walrus Debug Output");
#endif

    sContext = ContextCreate();
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_WALRUS_DIALOG), NULL, WndProc);
    ContextDestroy(sContext);
    return 0;
}
