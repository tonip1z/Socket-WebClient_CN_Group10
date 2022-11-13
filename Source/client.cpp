#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <thread>
#include <mutex> //stop the print result to be overlap from each thread, learn more: https://stackoverflow.com/questions/25848615/c-printing-cout-overlaps-in-multithreading
#include "client.h"

//Note to compiler: if you're using g++ to compile this code please add "-lws2_32" after "g++ -std=c++11 -pthread client.cpp [other files]"
//For example: "g++ -std=c++11 -pthread client.cpp -lws2_32"

//ref to winsock2.h example code: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
//ref to multithreading in C++: https://www.geeksforgeeks.org/multithreading-in-cpp/

#define PORT "80"

using namespace std;

mutex m;

int main(int argc, char* argv[])
{
    //Validate parameters (the aplication is used in command promt)
    if (argc < 2)
    {
        printf("Incorrect syntax. Please use: %s host-name/host-ip.\n", argv[0]);
        return 1;
    }

    //Initialize Winsock
    WSADATA wsaData;
    int WSAStartup_Result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (WSAStartup_Result != 0) {
        printf("WSAStartup failed with error: %d\n", WSAStartup_Result);
        return 1;
    }

    //Check if there is only one URL to be processed or there are multiple of them
    if (argc == 2) //only one URL
        process_address(argv[1], false);
    else if (argc > 2) //more than 1 URL, using multithreading to process all at the same time
    {
        thread connectionThread[4]; //support up to 4 connections at the same time
        int connections = min(4, argc - 1); //if user enter more than 4 URLs, only the first 4 are processed
        //this methods is to ensure hardware safety because different machines support different numbers of maximum threads

        for (int i = 0; i < connections; i++)
            connectionThread[i] = thread(process_address, argv[i + 1], true);

        for (int i = 0; i < connections; i++)
            connectionThread[i].join();
    }

    //Clean up
    WSACleanup();

    return 0;
}

void process_address(char* addr, bool multi_threaded)
{
    //Getting the host name from the URL
    char* host_name = getHostnameFromURL(addr);
    if (host_name == NULL)
    {
        m.lock();
        if (multi_threaded)
            cout << "[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
        cout << "Failed to retrieve host name.\n";
        m.unlock();
        return;
    }

    //sock_Connect is used for connecting to web servers
    SOCKET sock_Connect = INVALID_SOCKET; 

    //Create Internet address structure
    //learn more: https://learn.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
    //ref code: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_protocol = IPPROTO_TCP;
    
    //Resolve the server address and port
    //Please make sure your IP Routing is enabled on your Windows IP Configuration (to check: type ipconfig /all in cmd)
    //To enable IP Routing, see: https://www.wikihow.com/Enable-IP-Routing-on-Windows-10
    int getAddrInfo_Result = getaddrinfo(host_name, PORT, &hints, &result);
    if (getAddrInfo_Result != 0) 
    {
        if (getAddrInfo_Result == 11001)
        {
            m.lock();
            if (multi_threaded)
                cout << "[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
            cout << "Host not found.\nPlease make sure you entered the URL with HTTP or HTTPS protocol.\n";
            m.unlock();
            return;
        }
        
        m.lock();
        if (multi_threaded)
            cout << "[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
        printf("getaddrinfo failed with error: %d\n", getAddrInfo_Result);
        m.unlock();
        return;
    }

    //Initialize sock_Connect
    sock_Connect = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock_Connect == INVALID_SOCKET)
    {
        m.lock();
        if (multi_threaded)
            cout << "[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
        cout << "Failed to create socket.\n";
        m.unlock();
        return;
    }

    //Establish connection
    int connect_Result = connect(sock_Connect, result->ai_addr, (int)result->ai_addrlen);
    if (connect_Result == SOCKET_ERROR)
    {
        m.lock();
        if (multi_threaded)
            cout << "[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
        cout << "Connection failed.\n";
        m.unlock();
        closesocket(sock_Connect);
        sock_Connect = INVALID_SOCKET;
        return;
    }

    m.lock();
    if (multi_threaded)
        cout << "[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
    cout << "Connection successfully established.\n";
    cout << "Host name: " << host_name << "\n";
    cout << "Host IP: " << getIPv4(result->ai_addr) << "\n";
    m.unlock();
    
    //Send HTTP request

    //Clean up
    freeaddrinfo(result);
    delete[] host_name;
    closesocket(sock_Connect);
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

        if (secondSlash != NULL && thirdSlash != NULL)
        {
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
        else if (secondSlash != NULL && thirdSlash == NULL) //in case no thirdslash was found, i.e: https://www.google.com
        {
            return (secondSlash + 1);
        }
    }
    else //assume there is no "http://" or "https://" opening in the URL
    {
        char* firstSlash = strchr(URL, '/');

        if (firstSlash == NULL) //no path after host name
            return URL;

        string host_name = "";
        for (int i = 0; URL + i != firstSlash; i++)
            host_name += *(URL + i);

        int str_len = host_name.length();
        char* host_name_chr = new char[str_len + 1];
        
        for (int i = 0; i < str_len; i++)
            host_name_chr[i] = host_name[i];

        host_name_chr[str_len] = '\0';
        
        return host_name_chr;
    }

    return NULL;
}

//check if entered host-name/host-ip is entered as a URL starting with "http:" or "https:"
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