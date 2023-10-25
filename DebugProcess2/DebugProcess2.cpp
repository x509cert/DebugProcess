#define _CRT_SECURE_NO_WARNINGS (1)

#include <windows.h>
#include <stdio.h>

FILE* createLogFile() {
    char filename[64];
    DWORD millis = GetTickCount();

    snprintf(filename, sizeof(filename), "dbglogs\\log_%lu.txt", millis);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to create log file");
        exit(EXIT_FAILURE);
    }
    return fp;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_application>\n", argv[0]);
        return 1;
    }

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    FILE* logFile = createLogFile();

    if (!CreateProcessA(NULL,   // No module name (use command line)
        argv[1], // Command line
        NULL,   // Process handle not inheritable
        NULL,   // Thread handle not inheritable
        FALSE,  // Set handle inheritance to FALSE
        DEBUG_PROCESS, // Create in debug mode
        NULL,   // Use parent's environment block
        NULL,   // Use parent's starting directory 
        (LPSTARTUPINFOA) & si,    // Pointer to STARTUPINFO structure
        &pi))   // Pointer to PROCESS_INFORMATION structure
    {
        fprintf(logFile, "CreateProcess failed (%d).\n", GetLastError());
        fclose(logFile);
        return 1;
    }

    DEBUG_EVENT debugEv;
    DWORD continueStatus = DBG_CONTINUE;

    while (WaitForDebugEvent(&debugEv, 1500)) {
        switch (debugEv.dwDebugEventCode) {
        case EXCEPTION_DEBUG_EVENT:
            if (debugEv.u.Exception.ExceptionRecord.ExceptionCode != 0x80000003) {
                fprintf(logFile, "Exception code: %lx at address: %p\n",
                    debugEv.u.Exception.ExceptionRecord.ExceptionCode,
                    debugEv.u.Exception.ExceptionRecord.ExceptionAddress);
            }

            continueStatus = DBG_EXCEPTION_NOT_HANDLED;

            break;

        //case OUTPUT_DEBUG_STRING_EVENT:
        //{
        //    char s[1024];
        //    ReadProcessMemory(pi.hProcess,
        //        debugEv.u.DebugString.lpDebugStringData,
        //        s, debugEv.u.DebugString.nDebugStringLength,
        //        NULL);
        //    fprintf(logFile, "OutputDebugString: %s\n", s);
        //}
        //break;

        //case CREATE_THREAD_DEBUG_EVENT:
        //    fprintf(logFile, "Thread created with handle %p and ID %lu\n",
        //        debugEv.u.CreateThread.hThread, debugEv.dwThreadId);
        //    break;

        //case EXIT_THREAD_DEBUG_EVENT:
        //    fprintf(logFile, "Thread with ID %lu exited with code %lu\n",
        //        debugEv.dwThreadId, debugEv.u.ExitThread.dwExitCode);
        //    break;

        //case CREATE_PROCESS_DEBUG_EVENT:
        //    fprintf(logFile, "Process created with handle %p, thread handle %p, and process ID %lu\n",
        //        debugEv.u.CreateProcessInfo.hProcess,
        //        debugEv.u.CreateProcessInfo.hThread,
        //        debugEv.dwProcessId);
        //    break;

        //case EXIT_PROCESS_DEBUG_EVENT:
        //    fprintf(logFile, "Process with ID %lu exited with code %lu\n",
        //        debugEv.dwProcessId, debugEv.u.ExitProcess.dwExitCode);
        //    break;

        case RIP_EVENT:
            fprintf(logFile, "RIP event: Type %lu, Error %lu\n",
                debugEv.u.RipInfo.dwType, debugEv.u.RipInfo.dwError);
            break;

        }

        ContinueDebugEvent(debugEv.dwProcessId, debugEv.dwThreadId, continueStatus);
    }

    // Cleanup
    fclose(logFile);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
