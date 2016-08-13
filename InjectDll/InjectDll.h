// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 INJECTDLL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// INJECTDLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef INJECTDLL_EXPORTS
#define INJECTDLL_API __declspec(dllexport)
#else
#define INJECTDLL_API __declspec(dllimport)
#endif
#include "HookApi.h"

INJECTDLL_API void GetProcessPath(DWORD dwProcessID, void *buffer);
INJECTDLL_API HRESULT GetDllPath(LPTSTR lpstrOut, LPTSTR lpstrExecPath, LPTSTR lpstrDllName);
INJECTDLL_API DWORD DisplayErrorText();
INJECTDLL_API char* WcharToChar(const wchar_t* wp);
INJECTDLL_API wchar_t* CharToWchar(const char* c);
INJECTDLL_API HRESULT Panic(LPSTR message);
INJECTDLL_API HRESULT Prompt();

INJECTDLL_API extern TCHAR g_DllName[];
INJECTDLL_API extern TCHAR g_ExecPath[];
INJECTDLL_API extern TCHAR g_strWndClass[];
INJECTDLL_API extern TCHAR g_strWndName[];
INJECTDLL_API extern TCHAR g_strNamedPipe[];
INJECTDLL_API extern BOOL g_bInjected;
INJECTDLL_API extern HWND g_hWnd;
INJECTDLL_API extern HWND g_hWndTarget;
INJECTDLL_API extern HANDLE g_hProcess;
INJECTDLL_API extern DWORD g_dwBufSize;

void Attach(void);
void Detach(void);