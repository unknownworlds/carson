#include "Process.h"

#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#define BUFSIZE 4096 
 

HANDLE g_hInputFile = NULL;
 
void CreateChildProcess(void); 
void WriteToPipe(void); 
void ReadFromPipe(void); 
void ErrorExit(PTSTR); 

static bool ExecuteRemoteKernelFuntion(HANDLE process, const char* functionName, LPVOID param, DWORD& exitCode)
{

    HMODULE kernelModule = GetModuleHandle("Kernel32");
    FARPROC function = GetProcAddress(kernelModule, functionName);

    if (function == NULL)
    {
        return false;
    }

    DWORD threadId;
    HANDLE thread = CreateRemoteThread(process, NULL, 0,
        (LPTHREAD_START_ROUTINE)function, param, 0, &threadId);

    if (thread != NULL)
    {
        
        WaitForSingleObject(thread, INFINITE);
        GetExitCodeThread(thread, &exitCode);
        
        CloseHandle(thread);
        return true;

    }
    else
    {
        return false;
    }

}

bool Process_Run(void* userData, const char* command, Process_Callback callback, int* result)
{

    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;    

    SECURITY_ATTRIBUTES saAttr; 

    // Set the bInheritHandle flag so pipe handles are inherited. 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) 
    {
        return false;
    }
    // Create a pipe for the child process's STDIN. 
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
    {
        return false;
    }

    // Create the child process. 

    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE; 
 
    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    char* commandDup = _strdup(command);
    bSuccess = CreateProcess(NULL, commandDup, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &siStartInfo, &piProcInfo);

    if (!bSuccess)
    {
        // Try launching as a shell command.
        char* comspec = getenv("COMSPEC");
        if (comspec != NULL)
        {
            char commandLine[ MAX_PATH + 50 ];
            sprintf( commandLine, "%s /c %s", comspec, command );
            bSuccess = CreateProcess(NULL, commandLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &siStartInfo, &piProcInfo);
        }
    }
   
    // If an error occurs, exit the application. 
    if (!bSuccess) 
    {
        free(commandDup);
        return false;
    }

    // Disable Dr. Watson so that we don't get hung up if the process crashes.
    DWORD errorMode;
    ExecuteRemoteKernelFuntion(piProcInfo.hProcess, "SetErrorMode", (LPVOID)SEM_NOGPFAULTERRORBOX, errorMode);
    ExecuteRemoteKernelFuntion(piProcInfo.hProcess, "SetErrorMode", (LPVOID)(errorMode | SEM_NOGPFAULTERRORBOX), errorMode);

    ResumeThread(piProcInfo.hThread);

    CloseHandle(g_hChildStd_IN_Wr);
    CloseHandle(g_hChildStd_IN_Rd);

    DWORD dwRead; 
    CHAR chBuf[BUFSIZE]; 

    DWORD exitCode = STILL_ACTIVE;

    while (1)
    { 
        if (!GetExitCodeProcess(piProcInfo.hProcess, &exitCode))
        {
            break;
        }
        if (exitCode != STILL_ACTIVE)
        {
            if (!PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &dwRead, NULL) || dwRead == 0)
            {
                break;
            }
        }
        if (!PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &dwRead, NULL))
        {
            break;
        }
        if (dwRead != 0)
        {
            if (!ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL) || dwRead == 0 )
            {
                break; 
            }
            if (callback != NULL)
            {
                callback(userData, chBuf, dwRead);
            }
        }
    } 

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    free(commandDup);

    // The remaining open handles are cleaned up when this process terminates. 
    // To avoid resource leaks in a larger application, close handles explicitly. 

    CloseHandle(g_hChildStd_OUT_Rd);
    CloseHandle(g_hChildStd_OUT_Wr);
    
    if (result)
    {
        *result = exitCode;
    }

    return true; 

}