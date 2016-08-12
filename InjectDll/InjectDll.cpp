// InjectDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "InjectDll.h"
#include "HookApi.h"

INJECTDLL_API TCHAR g_DllName[] = _T("InjectDll.dll");
INJECTDLL_API TCHAR g_ExecPath[] = _T("taskmgr");
INJECTDLL_API TCHAR g_strWndClass[] = _T("TaskManagerWindow");
INJECTDLL_API TCHAR g_strWndName[] = _T("任务管理器");
INJECTDLL_API BOOL g_bInjected = FALSE;
INJECTDLL_API HWND g_hWnd = nullptr;
INJECTDLL_API HANDLE g_hProcess;

INJECTDLL_API void GetProcessPath(DWORD dwProcessID, void *buffer)
{
    TCHAR Filename[MAX_PATH];
    auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
    if (hProcess == nullptr)return;
    HMODULE hModule;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded))
    {
        if (GetModuleFileNameEx(hProcess, hModule, Filename, MAX_PATH)) {
            RtlMoveMemory(buffer, Filename, sizeof(char)*MAX_PATH);
        }
    }
    else {
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageName(hProcess, 0, Filename, &size)) {
            RtlMoveMemory(buffer, Filename, sizeof(char)*MAX_PATH);
        }
    }
    CloseHandle(hProcess);
}

INJECTDLL_API HRESULT GetDllPath(LPTSTR lpstrOut, LPTSTR lpstrExecPath, LPTSTR lpstrDllName)
{
    HRESULT hr;
    if ((hr = PathCchRemoveFileSpec(lpstrExecPath, MAX_PATH) != S_OK))
    {
        return hr;
    }
    if ((hr = PathCchCombine(lpstrOut, MAX_PATH, lpstrExecPath, lpstrDllName) != S_OK))
    {
        return hr;
    }
    return S_OK;
}

// 输出错误
INJECTDLL_API DWORD DisplayErrorText()
{
    HMODULE hModule = nullptr;
    LPSTR MessageBuffer;
    auto dwLastError = GetLastError();

    DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM;

    if ((FormatMessageA(
        dwFormatFlags,
        hModule,
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        LPSTR(&MessageBuffer),
        0,
        nullptr
        )))
    {
        printf("Error: %s\n", MessageBuffer);
        LocalFree(MessageBuffer);
    }

    return dwLastError;
}

INJECTDLL_API char* WcharToChar(const wchar_t* wp)
{
    char *m_char;
    int len = WideCharToMultiByte(CP_ACP, 0, wp, wcslen(wp), NULL, 0, NULL, NULL);
    m_char = new char[len + 1];
    WideCharToMultiByte(CP_ACP, 0, wp, wcslen(wp), m_char, len, NULL, NULL);
    m_char[len] = '\0';
    return m_char;
}

INJECTDLL_API wchar_t* CharToWchar(const char* c)
{
    wchar_t *m_wchar;
    int len = MultiByteToWideChar(CP_ACP, 0, c, strlen(c), NULL, 0);
    m_wchar = new wchar_t[len + 1];
    MultiByteToWideChar(CP_ACP, 0, c, strlen(c), m_wchar, len);
    m_wchar[len] = '\0';
    return m_wchar;
}

INJECTDLL_API HRESULT Panic(LPSTR message)
{
    printf("[%s] ", message);
    auto dwError = DisplayErrorText();
    system("pause");
    return dwError;
}

INJECTDLL_API HRESULT Prompt()
{
    HANDLE hToken;
    if (!OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        return Panic("OpenProcessToken");
    }
    TOKEN_PRIVILEGES tkp;
    tkp.PrivilegeCount = 1;
    if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tkp.Privileges[0].Luid))
    {
        return Panic("LookupPrivilegeValue");
    }
    if (!AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), nullptr, nullptr))
    {
        return Panic("AdjustTokenPrivileges");
    }
    return S_OK;
}

CHookApi* g_apiMsgBox;

int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    g_apiMsgBox->HookOff();
    std::wstringstream ss;
    ss << lpCaption << " - inject by bajdcc";
    auto nRet = ::MessageBoxW(hWnd, lpText, ss.str().c_str(), uType);
    g_apiMsgBox->HookOn(); 
    return nRet;
}

void Attach(void)
{
    g_hWnd = ::FindWindow(g_strWndClass, g_strWndName);
    if (g_hWnd == nullptr) return;
    DWORD dwPId;
    ::GetWindowThreadProcessId(g_hWnd, &dwPId);
    if (dwPId != ::GetCurrentProcessId())
        return;
    g_bInjected = true;
    char text[100];
    GetWindowTextA(g_hWnd, text, sizeof(text));
    std::stringstream ss;
    ss << text << " - injected by bajdcc";
    SetWindowTextA(g_hWnd, ss.str().c_str());
    g_hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());;
    assert(g_hProcess);
    g_apiMsgBox = CHookApi::Create("User32", "MessageBoxW", FARPROC(MyMessageBoxW));
    MessageBoxW(g_hWnd, L"Test", L"Test", MB_OK);
}

void Detach(void)
{
    if (!g_bInjected)
        return;
    if (g_hProcess)
        ::CloseHandle(g_hProcess);
    CHookApi::Clean();
}