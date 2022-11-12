#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include "client.h"

//Note to compiler: if you're using g++ to compile this code please add "-lws2_32" after "g++ client.cpp [other files]"
//For example: "g++ client.cpp -lws2_32"

//ref to winsock2.h example code: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code

#define PORT "80"

using namespace std;

int main(int argc, char* argv[])
{
    //Validate parameters (the aplication is used in command promt)
    if (argc != 2)
    {
        printf("Incorrect syntax. Please use: %s [HTTP or HTTPS URL].\n", argv[0]);
        return 1;
    }

    //Get host name from URL (argv[1])
    char* host_name = getHostnameFromURL(argv[1]);
    if (host_name == NULL)
    {
        //Cannot retrieve host name from URL
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
    int getAddrInfo_Result = getaddrinfo(host_name, PORT, &hints, &result);
    if (getAddrInfo_Result != 0) 
    {
        if (getAddrInfo_Result == 11001)
        {
            cout << "Host not found.\n";
            cout << "Please make sure you entered the URL with HTTP or HTTPS protocol.\n";
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

    cout << "Connection successfully established.\n";
    cout << "Host name: " << host_name << "\n";
    cout << "Host IP: " << getIPv4(result->ai_addr) << "\n";

    //Clean up
    delete[] host_name;
    closesocket(sock_Connect);
    WSACleanup();

    return 0;
}

//We specifically want to get a host name from the entered URL, because the program will take an URL as argument and the URL contains the content type (e.g "/" for index.html and "*.html", "*.pdf", etc for other file types)
//(e.g: get "web.stanford.edu" from "http://web.stanford.edu/dept/its/support/techtraining/techbriefing-media/Intro_Net_91407.ppt")
char* getHostnameFromURL(char* URL)
{
    if (is_HTTP_URL(URL))
    {
        char* secondSlash = strchr(URL, '/') + 1;
        char* thirdSlash = strchr(secondSlash + 1, '/');
        string host_name = "";

        //everything between second slash and third slash is the host name
        for (int i = 1; secondSlash + i != thirdSlash; i++)  
            host_name += *(secondSlash + i);
        
        int str_len = host_name.length();
        char* host_name_chr = new char[str_len + 1];
        
        for (int i = 0; i < str_len; i++)
            host_name_chr[i] = host_name[i];

        host_name_chr[str_len] = '\0';
        
        return host_name_chr;
    }
    else
    {
        cout << "Cannot retrieve host name. The entered URL is not of HTTP or HTTPS protocol.\n";
        return NULL;
    }
}

//check if the entered URL starting with "http:" or "https:" or not
bool is_HTTP_URL(char* host_name)
{
    if (host_name[0] == 'h' && host_name[1] == 't' && host_name[2] == 't' && host_name[3] == 'p')
        return true;
    
    return false;
}

//convert sockaddr to string, getting the IPv4 representation of a sockaddr
//ref code: https://stackoverflow.com/questions/1276294/getting-ipv4-address-from-a-sockaddr-structure, more specifically, ans: https://stackoverflow.com/a/32899053
string getIPv4(sockaddr* addr)
{
    char ipv4[INET_ADDRSTRLEN];
    char client_service[32];

    getnameinfo(addr, sizeof(*addr), ipv4, INET_ADDRSTRLEN, client_service, 32, NI_NUMERICHOST|NI_NUMERICSERV);

    return string(ipv4);
}
