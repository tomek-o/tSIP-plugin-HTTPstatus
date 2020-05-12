#define _EXPORTING
#include "..\tSIP\tSIP\phone\Phone.h"
#include "..\tSIP\tSIP\phone\PhoneSettings.h"
#include "..\tSIP\tSIP\phone\PhoneCapabilities.h"

#include "main.h"
#include <winsock.h>
#include <time.h>
#include <assert.h>

#define NETWORK_ERROR -1
#define NETWORK_OK     0

HANDLE Thread;
HANDLE SessionMutex;

static const struct S_PHONE_DLL_INTERFACE dll_interface =
{DLL_INTERFACE_MAJOR_VERSION, DLL_INTERFACE_MINOR_VERSION};

// callback ptrs
CALLBACK_LOG lpLogFn = NULL;
CALLBACK_CONNECT lpConnectFn = NULL;
CALLBACK_KEY lpKeyFn = NULL;
CALLBACK_ADD_TRAY_MENU_ITEM lpAddTrayMenuItemFn = NULL;

void *callbackCookie;	///< used by upper class to distinguish library instances when receiving callbacks


CALLBACK_SET_APP_STATUS lpSetAppStatusFn = NULL;

DWORD WINAPI ThreadProc(LPVOID data)
{
	int nret;

    while (1)
    {
        //OutputDebugStr("tcp status plugin is running");
        // Create the socket
        SOCKET theSocket = socket(AF_INET,		// Go over TCP/IP
                   SOCK_STREAM,			// stream-oriented socket
                   IPPROTO_TCP);		// Use TCP

        if (theSocket == INVALID_SOCKET) {
            nret = WSAGetLastError();
            //ReportError(nret, "socket()");
            Sleep(30000);
            continue;
            //return (DWORD)NETWORK_ERROR;
        }

        struct hostent *host;
        host = gethostbyname("tomeko.net");
        if (host == NULL) {
            Sleep(30000);
            continue;
        }
        SOCKADDR_IN SockAddr;
        SockAddr.sin_port=htons(80);
        SockAddr.sin_family=AF_INET;
        SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
        if(connect(theSocket,(SOCKADDR*)(&SockAddr),sizeof(SockAddr)) != 0){
            nret = WSAGetLastError();
            //ReportError(nret, "connect()");
            OutputDebugStr("connect error");
            Sleep(30000);
            continue;
        }
        // Successfully connected!

        const char* get = "GET /thermometer.txt HTTP/1.1\r\nHost: tomeko.net\r\nConnection: close\r\n\r\n";
        send(theSocket,get,strlen(get),0);

        char tmpbuf[1065];
        memset(tmpbuf, 0, sizeof(tmpbuf));
        nret = recv(theSocket, tmpbuf,
            1064,		// Complete size of buffer
            0);
        //char* ptr = strstr(tmpbuf, "\r\n\r\n");
        //Transfer-Encoding: chunked...
        /*
HTTP/1.1 200 OK
Content-Type: text/plain
Server: Apache
Last-Modified: Wed, 01 Jul 2015 20:38:30 GMT
Vary: Accept-Encoding
Transfer-Encoding: chunked
Date: Wed, 01 Jul 2015 20:44:39 GMT
Age: 119
Connection: close
X-Geo: varn32.rbx5
X-Geo-Port: 1014
X-Cacheable: Cacheable: matched cache

0024
Temp: 15.4C  RH: 99%  1026hPa  22:38
0


        */
        char* ptr = strstr(tmpbuf, "Temp:");
        if (ptr)
        {
            //ptr += 4; // \r\n\r\n
            if (nret == SOCKET_ERROR)
            {
                //return NETWORK_ERROR;
            }
            else
            {
                // nret contains the number of bytes received
                char * term = strchr(ptr, '\r');
                if (term)
                {
                    *term = '\0';
                }

                if (lpSetAppStatusFn)
                {
                    lpSetAppStatusFn(callbackCookie, "tcpstatus", 10, ptr);
                }
            }
        }

        // Send/receive, then cleanup:
        closesocket(theSocket);
        Sleep(150000);
    }
    return 0;
}

/** get handle to dll without knowing its name
*/
HMODULE GetCurrentModule()
{
    MEMORY_BASIC_INFORMATION mbi;
    static int dummy;
    VirtualQuery( &dummy, &mbi, sizeof(mbi) );
    return reinterpret_cast<HMODULE>(mbi.AllocationBase);
}

/* extern "C" __declspec(dllexport) */ void GetPhoneInterfaceDescription(struct S_PHONE_DLL_INTERFACE* interf) {
    interf->majorVersion = dll_interface.majorVersion;
    interf->minorVersion = dll_interface.minorVersion;
}

void Log(const char* txt) {
    if (lpLogFn)
        lpLogFn(callbackCookie, const_cast<char*>(txt));
}

void SetCallbacks(void *cookie, CALLBACK_LOG lpLog, CALLBACK_CONNECT lpConnect, CALLBACK_KEY lpKey) {
    assert(cookie && lpLog && lpConnect && lpKey);
    lpLogFn = lpLog;
    lpConnectFn = lpConnect;
    lpKeyFn = lpKey;
    callbackCookie = cookie;

    Log("SystemShutdown plugin loaded\n");
}

void GetPhoneCapabilities(struct S_PHONE_CAPABILITIES **caps) {
    static struct S_PHONE_CAPABILITIES capabilities = {
        0
    };
    *caps = &capabilities;
}

void ShowSettings(HANDLE parent) {
    MessageBox((HWND)parent, "No additional settings.", "Device DLL", MB_ICONINFORMATION);
}

int Connect(void)
{
    DWORD dwtid;

  	WORD sockVersion;
	WSADATA wsaData;

    sockVersion = MAKEWORD(1, 1);
    // Initialize Winsock
    WSAStartup(sockVersion, &wsaData);

    Thread = CreateThread(NULL,0,ThreadProc,0,0,&dwtid);
    return 0;
}

int Disconnect(void)
{
    TerminateThread(Thread,0);
    WSACleanup();
    return 0;
}

void SetCallbackSetAppStatus(CALLBACK_SET_APP_STATUS lpFn)
{
    lpSetAppStatusFn = lpFn;
}
