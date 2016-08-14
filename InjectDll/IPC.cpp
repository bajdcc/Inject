#include "stdafx.h"
#include "InjectDll.h"
#include "IPC.h"
#include <mutex>

HANDLE hPipe;
std::mutex mtx;

void Print(LPCSTR pstrMessage)
{
    if (!hPipe) return;
    DWORD dwWritten;
    if (!WriteFile(hPipe, pstrMessage, strlen(pstrMessage), &dwWritten, nullptr))
        return;
    FlushFileBuffers(hPipe);
}

HRESULT InitIPC()
{
    hPipe = CreateFile(g_strNamedPipe, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    
    if (INVALID_HANDLE_VALUE == hPipe)
    {
        hPipe = nullptr;
        return Panic("CreateFile INVALID_HANDLE_VALUE");
    }
    
    if (GetLastError() != ERROR_SUCCESS)
    {
        hPipe = nullptr;
        return Panic("CreateFile");
    }

    return S_OK;
}

HRESULT StartIPC(BOOL bLock)
{
    if (bLock)
        mtx.lock();
    return InitIPC();
}

HRESULT StopIPC(BOOL bLock)
{
    if (hPipe)
    {
        CloseHandle(hPipe);
        hPipe = nullptr;
    }
    if (bLock)
        mtx.unlock();
    return S_OK;
}