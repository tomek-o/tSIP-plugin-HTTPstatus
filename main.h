#include <windows.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define StartsWith(a,b) ((strlen(a) < strlen(b)) ? 0 : !memcmp(a,b,strlen(b)))

typedef int (*MSG_CALLBACK)(char*,char*);

__declspec(dllexport) int Load(MSG_CALLBACK callback);
__declspec(dllexport) int Unload();
__declspec(dllexport) char* Name();
__declspec(dllexport) int HandleMessage(char* msg);

DWORD WINAPI ThreadProc(LPVOID data);

#ifdef __cplusplus
}
#endif
