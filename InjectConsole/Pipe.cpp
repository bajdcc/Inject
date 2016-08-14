#include "stdafx.h"
#include "Pipe.h"

HRESULT ReadingFromClient()
{
    printf("创建管道!\n");

    //创建管道
    auto hPipe = CreateNamedPipe(g_strNamedPipe, PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, g_dwBufSize, g_dwBufSize,
        NMPWAIT_USE_DEFAULT_WAIT, nullptr);

    if (INVALID_HANDLE_VALUE == hPipe)
    {
        hPipe = nullptr;
        return Panic("CreateNamedPipe");
    }

    g_bPipe = TRUE;
    printf("管道句柄: 0x%p\n", hPipe);

    while (g_bPipe)
    {
        printf("[管道] 等待连接...\n");

        //等待客户端
        if (!ConnectNamedPipe(hPipe, nullptr))
        {
            CloseHandle(hPipe);
            return Panic("ConnectNamedPipe");
        }

        printf("[管道] 管道连接成功!\n");

        //读取管道中的数据
        DWORD nReadNum;
        auto hHeap = GetProcessHeap();
        auto szReadBuf = static_cast<char*>(HeapAlloc(hHeap, 0, (g_dwBufSize + 1)*sizeof(char)));
        while (ReadFile(hPipe, szReadBuf, g_dwBufSize, &nReadNum, nullptr))
        {
            if (nReadNum == 7 && strncmp(szReadBuf, "Detach!", 7) == 0)
                g_bPipe = FALSE;
            szReadBuf[nReadNum] = '\0';
            printf("[管道] >> %s\n", szReadBuf);
        }
        HeapFree(hHeap, 0, szReadBuf);
        printf("[管道] 关闭连接\n");

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
    }
    
    printf("[管道] 关闭管道\n");
    CloseHandle(hPipe);

    return S_OK;
}