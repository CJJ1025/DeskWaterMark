// DeskWaterMark.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "DeskWaterMark.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#define MAX_LOADSTRING 100

NOTIFYICONDATA nid; // 托盘图标信息
HMENU hMenu;        // 右键菜单句柄
// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 新增助手函数声明
void UpdateOverlay(HWND hWnd);
SYSTEMTIME GetGaokaoTarget();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DESKWATERMARK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DESKWATERMARK));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

// 设置程序开机自启
void SetAutoRun()
{
    WCHAR szExePath[MAX_PATH] = { 0 };
    // 获取当前exe完整路径
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);

    HKEY hKey;
    // 打开开机自启注册表项
    RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey);

    // 写入键名和路径，键名随便起，比如 DeskWaterMark
    RegSetValueExW(hKey, L"DeskWaterMark", 0, REG_SZ,
        (LPBYTE)szExePath, (wcslen(szExePath) + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);
}
//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DESKWATERMARK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DESKWATERMARK));
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   // 创建一个无边框的透明图层窗口，设置为顶层但不激活并且可穿透鼠标
   DWORD exStyles = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | 0x08000000; // WS_EX_NOACTIVATE = 0x08000000
   HWND hWnd = CreateWindowExW(exStyles, szWindowClass, szTitle, WS_POPUP,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // 初始化托盘图标
   ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
   nid.cbSize = sizeof(NOTIFYICONDATA);
   nid.hWnd = hWnd;
   nid.uID = 1001; // 自定义图标ID
   nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
   nid.uCallbackMessage = WM_USER + 100; // 自定义消息ID
   // 用我们之前写的代码图标，或者你也可以用LoadIcon加载ico
   nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DESKWATERMARK));
   // 托盘提示文字，鼠标悬停会显示
   wcscpy_s(nid.szTip, L"DeskWaterMark 高考倒计时");

   Shell_NotifyIcon(NIM_ADD, &nid);

   // 创建右键菜单
   hMenu = CreatePopupMenu();
   AppendMenu(hMenu, MF_STRING, 1002, L"退出程序");

   // 每秒刷新一次以保证时间及时更新
   SetTimer(hWnd, 1, 1000, NULL);

   SetAutoRun();//设置开机自启动
   return TRUE;
}

// 计算目标高考时间：默认使用当年6月7日09:00；若已过则使用下一年
SYSTEMTIME GetGaokaoTarget()
{
    SYSTEMTIME now;
    GetLocalTime(&now);
    SYSTEMTIME target = now;
    target.wMonth = 6; // 6月
    target.wDay = 7;   // 7号
    target.wHour = 9;  // 上午9点
    target.wMinute = 0;
    target.wSecond = 0;

    // 如果已经过去，则用下一年
    FILETIME ftNow, ftTarget;
    SystemTimeToFileTime(&now, &ftNow);
    SystemTimeToFileTime(&target, &ftTarget);
    ULARGE_INTEGER a, b;
    a.LowPart = ftNow.dwLowDateTime; a.HighPart = ftNow.dwHighDateTime;
    b.LowPart = ftTarget.dwLowDateTime; b.HighPart = ftTarget.dwHighDateTime;
    if (a.QuadPart >= b.QuadPart) {
        target.wYear = now.wYear + 1;
    }
    else {
        target.wYear = now.wYear;
    }
    return target;
}


void UpdateOverlay(HWND hWnd)
{
    // Prepare text
    SYSTEMTIME now;
    GetLocalTime(&now);
    SYSTEMTIME target = GetGaokaoTarget();

    FILETIME ftNow, ftTarget;
    SystemTimeToFileTime(&now, &ftNow);
    SystemTimeToFileTime(&target, &ftTarget);

    // compute difference in seconds
    ULARGE_INTEGER a, b;
    a.LowPart = ftNow.dwLowDateTime; a.HighPart = ftNow.dwHighDateTime;
    b.LowPart = ftTarget.dwLowDateTime; b.HighPart = ftTarget.dwHighDateTime;
    // FILETIME is in 100-nanosecond intervals since 1601. Convert to seconds
    long long diff100ns = (long long)(b.QuadPart - a.QuadPart);
    if (diff100ns < 0) diff100ns = 0;
    long long diffSec = diff100ns / 10000000LL;

    long long days = diffSec / 86400;
    long long hours = (diffSec % 86400) / 3600;
    long long minutes = (diffSec % 3600) / 60;
    long long seconds = diffSec % 60;

    wchar_t text[256];
    // 显示格式：距离高考还有 xx天 xx时 xx分
    swprintf_s(text, L"距离高考 %lld天 %lld时 %lld分 %lld秒", days, hours, minutes , seconds );

    // Font and metrics
    HDC hdcTemp1 = GetDC(NULL);
    int dpiY = GetDeviceCaps(hdcTemp1, LOGPIXELSY);
    ReleaseDC(NULL, hdcTemp1);

    int fontSize = -MulDiv(24, dpiY, 72); // 18pt
    HFONT hFont = CreateFontW(fontSize, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"KaiTi");

    // Prepare DIB section
    int padding = 16; // margin from edges
    SIZE sz;
    // calculate text rect
    HDC hdcMem = CreateCompatibleDC(NULL);
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 1; // temp
    bmi.bmiHeader.biHeight = 1;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    // create a temporary 1x1 DIB to measure text
    void* pvBits = NULL;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    HGDIOBJ hOldBmp = SelectObject(hdcMem, hBmp);
    HGDIOBJ hOldFont = SelectObject(hdcMem, hFont);
    SetTextColor(hdcMem, RGB(255,255,255));
    SetBkMode(hdcMem, TRANSPARENT);
    RECT rc = {0,0,0,0};
    DrawTextW(hdcMem, text, -1, &rc, DT_CALCRECT);

    int textWidth = rc.right - rc.left;
    int textHeight = rc.bottom - rc.top;

    SelectObject(hdcMem, hOldBmp);
    SelectObject(hdcMem, hOldFont);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);

    sz.cx = textWidth + padding*2;
    sz.cy = textHeight + padding*2;

    // Create actual DIB with calculated size
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = sz.cx;
    bmi.bmiHeader.biHeight = -sz.cy; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    pvBits = NULL;
    hdcMem = CreateCompatibleDC(NULL);
    hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    if (!hBmp || !pvBits) {
        DeleteDC(hdcMem);
        DeleteObject(hFont);
        return;
    }
    HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

    // Clear to fully transparent
    memset(pvBits, 0, sz.cx * sz.cy * 4);

    // Draw text in white onto DIB
    HGDIOBJ hOldF = SelectObject(hdcMem, hFont);
    SetTextColor(hdcMem, RGB(255,255,255));
    SetBkMode(hdcMem, TRANSPARENT);

    RECT drawRc = { padding, padding, padding + textWidth, padding + textHeight };
    // Use DrawText to render anti-aliased text
    DrawTextW(hdcMem, text, -1, &drawRc, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX);

    // Make alpha channel: for any pixel with non-zero RGB, set alpha = 220, else alpha = 0
    unsigned char* pixels = (unsigned char*)pvBits;
    int total = sz.cx * sz.cy;
    for (int i = 0; i < total; ++i) {
        unsigned char* p = pixels + i*4;
        // DIB section in BI_RGB with 32bpp is in BGRA order on little-endian
        unsigned char b = p[0], g = p[1], r = p[2];
        if (r != 0 || g != 0 || b != 0) {
            p[3] = 220; // alpha
            // Optional: lighten text color if you want colored text
            // keep white as is
        } else {
            p[3] = 0;
        }
    }

    // Prepare blend and call UpdateLayeredWindow
    POINT ptSrc = {0,0};
    RECT rcWork;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);

    int margin = 20;
    POINT ptWindow;
    ptWindow.x = rcWork.right - sz.cx - margin;
    ptWindow.y = rcWork.bottom - sz.cy - margin;

    SIZE sizeWindow = { sz.cx, sz.cy };

    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255; // use per-pixel alpha
    blend.AlphaFormat = AC_SRC_ALPHA;

    HDC hdcTemp2 = GetDC(NULL);
    UpdateLayeredWindow(hWnd, hdcTemp2, &ptWindow, &sizeWindow, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, hdcTemp2);

    // cleanup
    SelectObject(hdcMem, hOldF);
    SelectObject(hdcMem, hOld);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    DeleteObject(hFont);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        // 托盘图标消息
    case WM_USER + 100:
    {
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);

            // 创建临时右键菜单
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1002, L"退出程序");

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            PostMessage(hWnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
        break;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        // 托盘菜单 - 退出
        if (wmId == 1002)
        {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            DestroyWindow(hWnd);
            return 0;
        }

        // 原有菜单逻辑
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CREATE:
        UpdateOverlay(hWnd);
        break;

    case WM_TIMER:
        if (wParam == 1) {
            UpdateOverlay(hWnd);
        }
        break;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
