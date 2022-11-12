#pragma once
#include <windows.h>
#include <string>
#include <cstring>
#include <WinSock2.h>
#include <ws2tcpip.h>

using namespace std;

//Link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

//support functions
char* getHostnameFromURL(char* URL);
bool is_HTTP_URL(char* host_name);
string getIPv4(sockaddr* addr);