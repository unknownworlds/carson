#ifndef PROCESS_H
#define PROCESS_H

typedef void (Process_Callback)(void* userData, const char* string, size_t length);

bool Process_Run(void* userData, const char* command, Process_Callback outCallback, int* result);

#endif