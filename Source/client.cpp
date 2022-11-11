#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

//ref to winsock2.h example code: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
//Link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define PORT "80"

using namespace std;

int main(int argc, char* argv[])
{
    //Validate parameters (the aplication is used in command promt)
    if (argc != 2)
    {
        printf("Incorrect syntax. Please use: %s host-name/host-ip.\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;

    //sock_Connect is used for connecting to web servers
    SOCKET sock_Connect;

    //Create Internet address structure
    //learn more: https://learn.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
    //ref code: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    //Initialize Winsock
    int WSAStartup_Result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (WSAStartup_Result != 0) {
        printf("WSAStartup failed with error: %d\n", WSAStartup_Result);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //Resolve the server address and port
    int getAddrInfo_Result = getaddrinfo(argv[1], PORT, &hints, &result);
    if (getAddrInfo_Result != 0) 
    {
        if (getAddrInfo_Result == 11001)
        {
            cout << "Host not found.\n";
            WSACleanup();
            return 1;
        }
        
        printf("getaddrinfo failed with error: %d\n", getAddrInfo_Result);
        WSACleanup();
        return 1;
    }

    //Initialize sock_Connect
    sock_Connect = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock_Connect == INVALID_SOCKET)
    {
        cout << "Failed to create socket.\n";
        return 1;
    }

    cout << "Socket created successfully.\n";
    return 0;
}