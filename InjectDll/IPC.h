#pragma once

void Print(LPCSTR pstrMessage);
HRESULT StartIPC(BOOL bLock = TRUE);
HRESULT StopIPC(BOOL bLock = TRUE);