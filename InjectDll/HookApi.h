#pragma once
#include <vector>

class CHookApi
{
private:
    static std::vector<CHookApi*> vecHookApis;

public:
    static CHookApi* Create(LPCSTR lpModuleName, LPCSTR lpProcName, FARPROC fpHookProc);
    static void Clean();

protected:
    FARPROC OldProc;
    FARPROC NewProc;
    BYTE OldCode[5];
    BYTE NewCode[5];

    static void Patch(LPVOID lpAddress, LPCVOID lpBuffer, SIZE_T nSize);

public:
    CHookApi(LPCSTR lpModuleName, LPCSTR lpProcName, FARPROC fpHookProc);
    ~CHookApi();

    CHookApi(const CHookApi& api) = delete;

    void HookOn() const;
    void HookOff() const;
    FARPROC GetOldProc() const;
};