#include "stdafx.h"
#include "HookApi.h"
#include "InjectDll.h"

std::vector<CHookApi*> CHookApi::vecHookApis;

CHookApi::CHookApi(LPCSTR lpModuleName, LPCSTR lpProcName, FARPROC fpHookProc)
    : NewProc(fpHookProc)
{
    auto hmod = ::GetModuleHandleA(lpModuleName);
    if (!hmod)
        hmod = ::LoadLibraryA(lpModuleName);
    OldProc = FARPROC(::GetProcAddress(hmod, lpProcName));
    assert(OldProc);
    if (!OldProc) return;
    CopyMemory(OldCode, LPBYTE(OldProc), 5);
    NewCode[0] = 0xe9;
    *LPDWORD(NewCode + 1) = LPBYTE(fpHookProc) - LPBYTE(OldProc) - 5;

    HookOn();
}

CHookApi::~CHookApi()
{
    HookOff();
}

CHookApi* CHookApi::Create(LPCSTR lpModuleName, LPCSTR lpProcName, FARPROC fpHookProc)
{
    auto api = new CHookApi(lpModuleName, lpProcName, fpHookProc);
    vecHookApis.push_back(api);
    return api;
}

void CHookApi::Clean()
{
    for (auto& api : vecHookApis)
    {
        delete api;
    }
    vecHookApis.clear();
}

void CHookApi::Patch(LPVOID lpAddress, LPCVOID lpBuffer, SIZE_T nSize)
{
    DWORD dwTemp = 0;
    DWORD dwOldProtect;
    VirtualProtectEx(g_hProcess, lpAddress, nSize, PAGE_READWRITE, &dwOldProtect);
    WriteProcessMemory(g_hProcess, lpAddress, lpBuffer, nSize, nullptr);
    VirtualProtectEx(g_hProcess, lpAddress, nSize, dwOldProtect, &dwTemp);
}

void CHookApi::HookOn() const
{
    Patch(OldProc, NewCode, 5);
}

void CHookApi::HookOff() const
{
    Patch(OldProc, OldCode, 5);
}

FARPROC CHookApi::GetOldProc() const
{
    return OldProc;
}
