// InjectConsole.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <thread>
#include "Pipe.h"

int main()
{
    HRESULT hr;

    printf("程序初始化中...\n");
    if ((hr = Prompt()) != S_OK)
    {
        return hr;
    }
    printf("提权成功!\n");

    //可执行文件路径
    TCHAR lpstrPath[MAX_PATH];
    GetProcessPath(GetCurrentProcessId(), lpstrPath);
    printf("程序路径: %S\n", lpstrPath);
    //注入dll路径
    TCHAR szLibPath[MAX_PATH];
    if (GetDllPath(szLibPath, lpstrPath, g_DllName) != S_OK)
    {
        printf("找不到路径\n");
        system("pause");
    }
    printf("注入路径: %S\n", szLibPath);
    //取得DLL模板句柄
    //取得Kernel32.dll的函数地址
    auto hKernel32 = ::GetModuleHandleA("Kernel32");
    printf("(HMODULE) Kernel32.dll:     0x%p\n", hKernel32);
    //取得fpcLoadLibrary的函数地址
    auto fpcLoadLibrary = ::GetProcAddress(hKernel32, "LoadLibraryA");
    printf("(FARPROC) LoadLibrary:      0x%p\n", fpcLoadLibrary);
    //取得fpcFreeLibrary的函数地址
    auto fpcFreeLibrary = ::GetProcAddress(hKernel32, "FreeLibrary");
    printf("(FARPROC) FreeLibrary:      0x%p\n", fpcFreeLibrary);
    //取得fpcGetConsoleWindow的函数地址
    auto fpcGetConsoleWindow = GetProcAddress(hKernel32, "GetConsoleWindow");
    printf("(FARPROC) GetConsoleWindow: 0x%p\n", fpcGetConsoleWindow);
    typedef HWND(FAR WINAPI *pGetConsoleWindow)();
    //控制台窗口句柄
    auto hwConsoleWindow = pGetConsoleWindow(fpcGetConsoleWindow)();
    char lpstrConsoleTitle[MAX_PATH];

    ::SetConsoleTitleA("Inject Test");
    ::GetConsoleTitleA(lpstrConsoleTitle, MAX_PATH);

    printf("窗口句柄: 0x%p\n", hwConsoleWindow);
    printf("窗口标题: %s\n", lpstrConsoleTitle);
    printf("进程句柄: 0x%08X\n", GetCurrentProcessId());
    printf("线程句柄: 0x%08X\n", GetCurrentThreadId());
    auto hStd_OP = ::GetStdHandle(STD_OUTPUT_HANDLE);
    printf("标准输出: 0x%p\n", hStd_OP);

    if (int(::ShellExecute(nullptr, _T("open"), g_ExecPath, nullptr, nullptr, SW_SHOWNORMAL)) > 32)
    {
        printf("窗口已打开!\n");
    }
    else
    {
        return Panic("ShellExecuteA");
    }

    auto hWnd = ::FindWindow(g_strWndClass, g_strWndName);
    while (!hWnd)
    {
        hWnd = ::FindWindow(g_strWndClass, g_strWndName);
        ::Sleep(1000);
    }

    printf("窗口句柄: 0x%p\n", hWnd);

    HANDLE hRemoteProcess;
    DWORD dwPId;
    DWORD dwTId;
    DWORD dwPathLength;
    HANDLE hThread;
    DWORD hLibModule;
    LPVOID pLibRemoteSrc;

    dwTId = ::GetWindowThreadProcessId(hWnd, &dwPId);
    printf("进程句柄: 0x%08X\n", dwPId);
    printf("线程句柄: 0x%08X\n", dwTId);
    //获取远程进程的HANDLE句柄
    hRemoteProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPId);
    while (!hRemoteProcess)
    {
        hRemoteProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPId);
        ::Sleep(1000);
    }
    printf("打开进程: 0x%p\n", hRemoteProcess);

    //注入DLL路径
    auto szLibPathA = std::make_unique<char*>(WcharToChar(szLibPath));
    auto nLibPathLength = sizeof(char)*(strlen(*szLibPathA)) + 1;

    //在远程进程中为DLL名称分配内存空间
    if (!((pLibRemoteSrc = ::VirtualAllocEx(hRemoteProcess, nullptr, nLibPathLength, MEM_COMMIT, PAGE_READWRITE))))
    {
        return Panic("VirtualAllocEx");
    }
    else
    {
        printf("执行地址: 0x%p\n", pLibRemoteSrc);
    }
    //写入DLL名称(包括完整路径)到已分配的存储空间中
    if (!::WriteProcessMemory(hRemoteProcess, pLibRemoteSrc, LPVOID(*szLibPathA), nLibPathLength, &dwPathLength))
    {
        return Panic("WriteProcessMemory");
    }
    if (!::ReadProcessMemory(hRemoteProcess, pLibRemoteSrc, LPVOID(*szLibPathA), nLibPathLength, &dwPathLength))
    {
        return Panic("ReadProcessMemory");
    }
    printf("注入路径: %s\n", *szLibPathA);
    printf("路径长度: %d\n", dwPathLength);
    //映射DLL到远程进程中
    hThread = ::CreateRemoteThread(hRemoteProcess, nullptr, NULL,
        LPTHREAD_START_ROUTINE(fpcLoadLibrary), pLibRemoteSrc, NULL, nullptr);
    if (!hThread)
    {
        return Panic("CreateRemoteThread");
    }
    printf("线程句柄: 0x%p\n", hThread);
    //管道初始化
    ReadingFromClient();
    //等待远程线程终止
    ::WaitForSingleObject(hThread, INFINITE);
    //返回远程线程的退出代码
    ::GetExitCodeThread(hThread, &hLibModule);
    printf("模块句柄: 0x%08X\n", hLibModule);
    //清除Thread
    ::CloseHandle(hThread);
    ::VirtualFreeEx(hRemoteProcess, pLibRemoteSrc, MAX_PATH, MEM_RELEASE);
    hThread = nullptr;

    system("pause");

    //清理
    printf("关闭窗口...\n");
    //从目标进程中将DLL卸载
    hThread = ::CreateRemoteThread(hRemoteProcess, nullptr, NULL,
        LPTHREAD_START_ROUTINE(fpcFreeLibrary), LPVOID(hLibModule), NULL, nullptr);
    printf("卸载线程: 0x%p\n", hThread);
    //等待响应
    ::WaitForSingleObject(hThread, INFINITE);
    //释放VirtualAllocEx申请的内存
    printf("释放远程Dll...\n");
    ::VirtualFreeEx(hRemoteProcess, pLibRemoteSrc, MAX_PATH, MEM_RELEASE);
    ::CloseHandle(hThread);
    hThread = nullptr;
    printf("关闭窗口句柄...\n");
    CloseHandle(hRemoteProcess);
    hRemoteProcess = nullptr;
    SendMessage(hWnd, WM_CLOSE, NULL, NULL);
    hWnd = nullptr;

    printf("窗口已关闭\n");
    return 0;
}
