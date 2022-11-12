#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include "client.h"

//Note to compiler: if you're using g++ to compile this code please add "-lws2_32" after "g++ client.cpp [other files]"
//For example: "g++ client.cpp -lws2_32"

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
    SOCKET sock_Connect = INVALID_SOCKET; 

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
    //Please make sure your IP Routing is enabled on your Windows IP Configuration (to check: type ipconfig /all in cmd)
    //To enable IP Routing, see: https://www.wikihow.com/Enable-IP-Routing-on-Windows-10
    int getAddrInfo_Result = getaddrinfo(argv[1], PORT, &hints, &result);
    if (getAddrInfo_Result != 0) 
    {
        if (getAddrInfo_Result == 11001)
        {
            cout << "Host not found.\n";

            //ref: https://superuser.com/questions/326403/can-you-get-a-reply-from-a-https-site-using-the-ping-command
            if (is_HTTP_URL(argv[1]))
            {
                cout << "Cannot retrieve address from an URL. Please enter either a host-name or a host-ip.\n";
                cout << "(E.g: example.com is a host name, while https://example.com/ is not)\n";
            }

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

    //Establish connection
    int connect_Result = connect(sock_Connect, result->ai_addr, (int)result->ai_addrlen);
    if (connect_Result == SOCKET_ERROR)
    {
        cout << "Connection failed.\n";
        closesocket(sock_Connect);
        sock_Connect = INVALID_SOCKET;
        return 1;
    }

    cout << result->ai_addr << "\n";

    return 0;
}

//check if entered host-name/host-ip is entered as a URL starting with "http:" or "https:"
bool is_HTTP_URL(char* host_name)
{
    if (host_name[0] == 'h' && host_name[1] == 't' && host_name[2] == 't' && host_name[3] == 'p')
        return true;
    
    return false;
}