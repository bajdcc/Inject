#include "stdafx.h"
#include <thread>
#include "InjectDll.h"
#include "IPC.h"

HANDLE hPipe;

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

    Print("This is a Test Message!");

    return S_OK;
}

HRESULT StartIPC()
{
    return InitIPC();
}

HRESULT StopIPC()
{
    if (hPipe)
    {
        CloseHandle(hPipe);
        hPipe = nullptr;
    }
    return S_OK;
}