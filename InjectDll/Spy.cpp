#include "stdafx.h"
#include "InjectDll.h"
#include "IPC.h"
#include "stb_image.h"

#define RECT_WIDTH(rect) ((rect.right) - (rect.left))
#define RECT_HEIGHT(rect) ((rect.bottom) - (rect.top))
#define RECT_SIZE(rect) ((RECT_WIDTH(rect)) * (RECT_HEIGHT(rect)))

#define DISPLAY_R 17
#define DISPLAY_G 125
#define DISPLAY_B 187

#define BG_R 255
#define BG_G 255
#define BG_B 255

#define LINE_R 217
#define LINE_G 234
#define LINE_B 244

#define EDGE_R 17
#define EDGE_G 125
#define EDGE_B 189

#define FILL_R 241
#define FILL_G 246
#define FILL_B 250

#define MAX_FRAME 6574

#define REFRESH_RATE 33

BOOL g_bHooking = FALSE;
HHOOK g_hHook = nullptr;
HWND g_prevHwnd = nullptr;
HWND g_savedHwnd = nullptr;
UINT_PTR uTimer;
UINT_PTR uHook;
UINT_PTR uSDL;
WNDPROC oldProc;
BOOL bUpdate;

SDL_Window * sdlWindow = nullptr;
SDL_Renderer* sdlRenderer = nullptr;
SDL_Surface* sdlSurface = nullptr;

char lpszMsg[100] = { 0 };
char oldCaption[100] = { 0 };
DWORD lastTime;
DWORD beginTime;
DWORD pastTime;
double fps;

HWND SpyFindSmallestWindow(const POINT &pt)
{
    auto hWnd = WindowFromPoint(pt); // 鼠标所在窗口

    if (hWnd)
    {
        // 得到本窗口大小和父窗口句柄，以便比较
        RECT rect;
        ::GetWindowRect(hWnd, &rect);
        auto parent = ::GetParent(hWnd); // 父窗口

        // 只有该窗口有父窗口才继续比较
        if (parent)
        {
            // 按Z方向搜索
            auto find = hWnd; // 递归调用句柄
            RECT rect_find;

            while (1) // 循环
            {
                find = ::GetWindow(find, GW_HWNDNEXT); // 得到下一个窗口的句柄
                ::GetWindowRect(find, &rect_find); // 得到下一个窗口的大小

                if (::PtInRect(&rect_find, pt) // 鼠标所在位置是否在新窗口里
                    && ::GetParent(find) == parent // 新窗口的父窗口是否是鼠标所在主窗口
                    && ::IsWindowVisible(find)) // 窗口是否可视
                {
                    // 比较窗口，看哪个更小
                    if (RECT_SIZE(rect_find) < RECT_SIZE(rect))
                    {
                        // 找到更小窗口
                        hWnd = find;

                        // 计算新窗口的大小
                        ::GetWindowRect(hWnd, &rect);
                    }
                }

                // hWnd的子窗口find为NULL，则hWnd为最小窗口
                if (!find)
                {
                    break; // 退出循环
                }
            }
        }
    }

    return hWnd;
}

void SpyInvertBorder(const HWND &hWnd)
{
    // 若非窗口则返回
    if (!IsWindow(hWnd))
        return;

    RECT rect; // 窗口矩形

    // 得到窗口矩形
    ::GetWindowRect(hWnd, &rect);

    auto hDC = ::GetWindowDC(hWnd); // 窗口设备上下文

    // 设置窗口当前前景色的混合模式为R2_NOT
    // R2_NOT - 当前的像素值为屏幕像素值的取反，这样可以覆盖掉上次的绘图
    SetROP2(hDC, R2_NOT);

    // 创建画笔
    HPEN hPen;

    // PS_INSIDEFRAME - 产生封闭形状的框架内直线，指定一个限定矩形
    // 3 * GetSystemMetrics(SM_CXBORDER) - 三倍边界粗细
    // RGB(0,0,0) - 黑色
    hPen = ::CreatePen(PS_INSIDEFRAME, 3 * GetSystemMetrics(SM_CXBORDER), RGB(0, 0, 0));

    // 选择画笔
    auto old_pen = ::SelectObject(hDC, hPen);

    // 设定画刷
    auto old_brush = ::SelectObject(hDC, GetStockObject(NULL_BRUSH));

    // 画矩形
    Rectangle(hDC, 0, 0, RECT_WIDTH(rect), RECT_HEIGHT(rect));

    // 恢复原来的设备环境
    ::SelectObject(hDC, old_pen);
    ::SelectObject(hDC, old_brush);

    DeleteObject(hPen);
    ReleaseDC(hWnd, hDC);
}

void SpyExecScanning(POINT &pt)
{
    ClientToScreen(g_hWnd, &pt); // 转换到屏幕坐标

    auto current_window = SpyFindSmallestWindow(pt); //找到当前位置的最小窗口

    if (current_window)
    {
        // 若是新窗口，就把旧窗口的边界去掉，画新窗口的边界
        if (current_window != g_prevHwnd)
        {
            SpyInvertBorder(g_prevHwnd);
            g_prevHwnd = current_window;
            SpyInvertBorder(g_prevHwnd);
        }
    }

    g_savedHwnd = g_prevHwnd;
}

int nSDLTime;

void ProcessingImage(stbi_uc* data, int width, int height, int comp, int pitch)
{
    int i, j;
    BYTE c, prev;
    //二值化
    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++)
        {
            //auto B = data[j * pitch + i * comp];
            //auto G = data[j * pitch + i * comp + 1];
            //auto R = data[j * pitch + i * comp + 2];
            auto Gray = 0.212671f * data[j * pitch + i * comp + 2] +
                0.715160f * data[j * pitch + i * comp + 1] +
                0.072169f * data[j * pitch + i * comp];
            if (Gray < 128.0f)
            {
                data[j * pitch + i * comp] = 0;
                data[j * pitch + i * comp + 1] = 0;
                data[j * pitch + i * comp + 2] = 0;
            }
            else
            {
                data[j * pitch + i * comp] = 255;
                data[j * pitch + i * comp + 1] = 255;
                data[j * pitch + i * comp + 2] = 255;
            }
        }
    }
    //边缘检测
    prev = 0;
    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++)
        {
            c = data[j * pitch + i * comp];
            if (c != prev)
            {
                data[j * pitch + i * comp] = DISPLAY_B;
                data[j * pitch + i * comp + 1] = DISPLAY_G;
                data[j * pitch + i * comp + 2] = DISPLAY_R;
            }
            prev = c;
        }
    }
    //边缘检测
    prev = 0;
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            c = data[j * pitch + i * comp];
            if (c != prev)
            {
                data[j * pitch + i * comp] = DISPLAY_B;
                data[j * pitch + i * comp + 1] = DISPLAY_G;
                data[j * pitch + i * comp + 2] = DISPLAY_R;
            }
            prev = c;
        }
    }
    //背景
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            c = data[j * pitch + i * comp];
            if (c == DISPLAY_B)
                continue;
            if ((j % (height / 10) == 0) && j != 0)//横线
            {
                data[j * pitch + i * comp] = LINE_B;
                data[j * pitch + i * comp + 1] = LINE_G;
                data[j * pitch + i * comp + 2] = LINE_R;
            }
            else if ((i % (width / 5) == (((MAX_FRAME - nSDLTime) / 30 * (width / 20))) % (width / 5)) && i != 0)//竖线
            {
                data[j * pitch + i * comp] = LINE_B;
                data[j * pitch + i * comp + 1] = LINE_G;
                data[j * pitch + i * comp + 2] = LINE_R;
            }
            else if (c == 255)
            {
                data[j * pitch + i * comp] = BG_B;
                data[j * pitch + i * comp + 1] = BG_G;
                data[j * pitch + i * comp + 2] = BG_R;
            }
            else if (c == 0)
            {
                data[j * pitch + i * comp] = FILL_B;
                data[j * pitch + i * comp + 1] = FILL_G;
                data[j * pitch + i * comp + 2] = FILL_R;
            }
        }
    }
    //边框
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            if ((i == 0 || i == width - 1) || (j == 0 || j == height - 1))
            {
                data[j * pitch + i * comp] = EDGE_B;
                data[j * pitch + i * comp + 1] = EDGE_G;
                data[j * pitch + i * comp + 2] = EDGE_R;
            }
        }
    }
}

VOID CALLBACK SDLProc(HWND, UINT, UINT_PTR, DWORD)
{
    static char filename[100];

    if (bUpdate)
        nSDLTime++;
    else
        return;

    sprintf_s(filename, g_strImagePathFormat, nSDLTime);
    int x, y, comp;
    auto data = stbi_load(filename, &x, &y, &comp, 0);
    if (!data)
        return;

    ProcessingImage(data, x, y, comp, x * comp);

    auto image = SDL_CreateRGBSurfaceFrom(data, x, y, comp << 3, x * comp, 0, 0, 0, 0);
    if (!image)
    {
        SetWindowTextA(g_hWnd, SDL_GetError());
        return;
    }

    auto texture = SDL_CreateTextureFromSurface(sdlRenderer, image);

    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
    SDL_RenderPresent(sdlRenderer);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    stbi_image_free(data);

    if (nSDLTime > MAX_FRAME)
    {
        auto font = TTF_OpenFont("C:\\windows\\fonts\\msyh.ttf", 32);
        assert(font);
        SDL_Color color = { 17, 152, 187 };

        auto surface = TTF_RenderUNICODE_Blended(font, PUINT16(L"播放完毕，谢谢欣赏！"), color);
        texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);

        SDL_Rect rt;
        rt.x = 0;
        rt.y = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &rt.w, &rt.h);

        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, texture, &rt, &rt);
        SDL_RenderPresent(sdlRenderer);

        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
        TTF_CloseFont(font);

        SDL_Delay(2000);

        SDL_DestroyWindow(sdlWindow);
        SDL_DestroyRenderer(sdlRenderer);
        sdlWindow = nullptr;
        sdlRenderer = nullptr;
        SDL_Quit();

        SetWindowTextA(g_hWnd, oldCaption);

        if (uSDL)
            KillTimer(g_hWnd, uSDL);
        return;
    }

    {
        auto time = timeGetTime();
        auto sec = (time - beginTime) / 1000;
        if (sec > pastTime)
        {
            int span = time - lastTime;
            lastTime = time;
            pastTime = sec;
            fps = 1.0 * span / REFRESH_RATE;
            sprintf_s(lpszMsg, "[BAD APPLE] Time: %02d:%02d, Frame: %04d, FPS: %02.2f - bajdcc", sec / 60,
                sec % 60, nSDLTime, fps);
            SetWindowTextA(g_hWnd, lpszMsg);
            sprintf_s(lpszMsg, "[%02d:%02d] %04d %02.2f", sec / 60,
                sec % 60, nSDLTime, fps);
            StartIPC();
            Print(lpszMsg);
            StopIPC();
        }
    }
}

LRESULT CALLBACK PaintProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_PAINT)
        return TRUE;

    if (msg == WM_LBUTTONUP)
        bUpdate = !bUpdate;

    SDL_Event event;
    SDL_PollEvent(&event);

    return CallWindowProc(oldProc, hWnd, msg, wParam, lParam);
}

void Prepare()
{
    nSDLTime = 0;
    bUpdate = TRUE;
    lastTime = timeGetTime();
    beginTime = lastTime;
    pastTime = 0;
    fps = 0;
}

void InitSDL(HWND hWnd)
{
    oldProc = WNDPROC(SetWindowLong(hWnd, GWL_WNDPROC, LONG(PaintProc)));

    Print("Create SDL Window...");
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        Print("Create SDL Window... FAILED");
        Print(SDL_GetError());
        return;
    }

    sdlWindow = SDL_CreateWindowFrom(static_cast<void*>(hWnd));
    if (sdlWindow)
        Print("Create SDL Window... OK");
    else
    {
        Print("Create SDL Window... FAILED");
        return;
    }
    Print("Create SDL Surface... OK");

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdlRenderer)
    {
        Print("Create SDL Renderer... FAILED");
        return;
    }
    Print("Create SDL Renderer... OK");

    sdlSurface = SDL_GetWindowSurface(sdlWindow);
    if (!sdlSurface)
    {
        Print("Create SDL Surface... FAILED");
        return;
    }
    Print("Create SDL Surface... OK");

    if (TTF_Init() == -1)
    {
        Print("Create SDL TTF... FAILED");
        Print(TTF_GetError());
        return;
    }
    Print("Create SDL TTF... OK");

    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderPresent(sdlRenderer);

    auto font = TTF_OpenFont("C:\\windows\\fonts\\msyh.ttf", 32);
    assert(font);
    SDL_Color color = { 17, 152, 187 };

    auto surface = TTF_RenderUNICODE_Blended(font, PUINT16(L"准备播放动画！"), color);
    auto texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);

    SDL_Rect rt;
    rt.x = 0;
    rt.y = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &rt.w, &rt.h);

    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, texture, &rt, &rt);
    SDL_RenderPresent(sdlRenderer);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);

    SDL_Delay(2000);

    Prepare();

    GetWindowTextA(g_hWnd, oldCaption, sizeof(oldCaption));

    uSDL = SetTimer(g_hWnd, WM_USER + 402, REFRESH_RATE, SDLProc);
}

VOID CALLBACK HookProc(HWND, UINT, UINT_PTR, DWORD)
{
    StartIPC();
    Print("Find target window!");
    sprintf_s(lpszMsg, "HWND: %p", g_prevHwnd);
    Print(lpszMsg);
    InitSDL(g_prevHwnd);
    StopIPC();
    if (uHook)
    {
        KillTimer(g_hWnd, uHook);
        uHook = NULL;
    }
}

LRESULT CALLBACK MsgHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) {
        return CallNextHookEx(g_hHook, nCode, wParam, lParam);
    }

    if (nCode == HC_ACTION)
    {
        auto lpMsg = LPMSG(lParam);
        POINT pt;

        switch (lpMsg->message) {
        case WM_MOUSEMOVE:
            if (g_bHooking)
            {
                pt = lpMsg->pt;
                ScreenToClient(g_hWnd, &pt);
                SpyExecScanning(pt);
            }
            break;
        case WM_MBUTTONDOWN:
            if (g_bHooking)
            {
                pt = lpMsg->pt;
                g_prevHwnd = SpyFindSmallestWindow(pt); //找到当前位置的最小窗口
                g_bHooking = FALSE;
                uHook = SetTimer(g_hWnd, WM_USER + 401, 1000, TIMERPROC(HookProc));
            }
            break;
        default:
            break;
        }
    }

    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    if (g_bSpying) {
        return;
    }
    StartIPC();
    Print("Hooking...");
    g_bHooking = TRUE;
    g_hHook = SetWindowsHookEx(WH_GETMESSAGE, HOOKPROC(MsgHookProc),
        nullptr, GetWindowThreadProcessId(g_hWnd, nullptr));
    if (!g_hHook) {
        Print("Hooking... FAILED");
        PanicWithPipe("SetWindowsHookEx");
        StopIPC();
        return;
    }
    Print("Hooking... OK");
    StopIPC();
    g_bSpying = TRUE;
    if (uTimer)
    {
        KillTimer(g_hWnd, uTimer);
        uTimer = NULL;
    }
}

void StartSpy()
{
    if (!g_hWnd)
        return;
    uTimer = SetTimer(g_hWnd, WM_USER + 400, 1000, TIMERPROC(TimerProc));
}

void StopSpy()
{
    if (!g_hHook)
        return;
    Print("Unhooking...");
    g_bSpying = FALSE;
    if (!UnhookWindowsHookEx(g_hHook)) {
        Print("Unhooking... FAILED");
        return;
    }
    g_hHook = nullptr;
    Print("Unhooking... OK");
}
