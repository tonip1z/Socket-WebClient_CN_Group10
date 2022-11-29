#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
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
        printf("Incorrect syntax. Please use: %s [HTTP or HTTPS URL(s)].\n", argv[0]);
        return 1;
    }

    //Initialize Winsock
    WSADATA wsaData;
    int WSAStartup_Result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (WSAStartup_Result != 0) 
    {
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
            cout << "Host not found.\nPlease make sure:\n";
            cout << "- You have connected to Internet.\n";
            cout << "- You have 'IP Routing' enabled on your Windows IP Configuration (type 'ipconfig /all' in cmd to check).\n";
            cout << "- You have entered the URL with HTTP or HTTPS protocol.\n";
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
        cout << "\n[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
    cout << "Connection successfully established.\n";
    cout << "Host name: " << host_name << "\n";
    cout << "Host IP: " << getIPv4(result->ai_addr) << "\n";
    m.unlock();
    
    //Create HTTP message (initial buffer) and send it
    string GET_QUERY = create_GET_query(addr, host_name);
    const char* sendbuff = GET_QUERY.c_str();
    int byte_sent = send(sock_Connect, sendbuff, (int)strlen(sendbuff), 0);
    if (byte_sent == SOCKET_ERROR)
    {
        m.lock();
        if (multi_threaded)
            cout << "\n[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
        cout << "Failed to send HTTP message to server.\n";
        m.unlock();
        closesocket(sock_Connect);
        return;
    }

    m.lock();
    if (multi_threaded)
        cout << "\n[Thread " << this_thread::get_id() << "] - " << addr << ":\n";
    cout << "Sent HTTP request to '" << host_name << "' successfully.\n";
    cout << "Byte sent to server: " << byte_sent << "\n";
    m.unlock();

    cout << "\nDATA SENT:\n";
    cout << "---------------------------------------------------------\n";
    cout << sendbuff;
    cout << "\n---------------------------------------------------------\n";

    //Recieve data
    //ref code: https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
    int byte_recv;
    char recvbuff[1024];
    vector<string> headers;
    string line;
    string excess_data = "";

    //get status code
    int status_code;
    line = recvALineFromServerRepsonse(sock_Connect, headers); //first line of HTTP response contains a status code (e.g. 200, 501, 502, 404,...)
    getStatusCodeInfo(line, status_code);

    if (status_code == 200)
    {
        int content_length = -1; //if the HTTP response contains "Content-Length" header, store the content length
                                 //if the HTTP response contains "Transfer-Encoding: chunked", remains -1

        //recieve all the headers, extracts and put them into a vector
        while ((line.length() != 2) && (int(line[0]) != 13) && (int(line[1]) != 10)) //13: CR, 10: LF - '\r\n' in ASCII
            line = recvALineFromServerRepsonse(sock_Connect, headers);
        
        //Extract "Content-Length" or "Transfer-Encoding: chunked" from the vector headers
        cout << "\nDATA RECIEVED:\n";
        cout << "---------------------------------------------------------\n";
        for (int i = 0; i < headers.size(); i++)
        {
            cout << headers[i];
            if (headers[i].find("Content-Length") != string::npos)
            {
                content_length = getContentLength(headers[i]);
                break;
            }
        }
        cout << "(message body)\n";
        cout << "---------------------------------------------------------\n";

        downloadFile(sock_Connect, "Downloaded/index.html", content_length);
    }

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

string create_GET_query(char* addr, char* host_name)
{
    /*General template for simple GET HTTP request used:
    
        GET /index.html HTTP/1.1\r\n
        Host: example.com\r\n
        Connection: keep-alive\r\n
        \r\n
    */
    
    string host_name_str = host_name;
    string GET_query = "GET " + get_abs_path(addr, host_name) + " HTTP/1.1\r\nHost: " + host_name_str + "\r\nConnection: keep-alive\r\n\r\n";
    
    return GET_query;
}

string get_abs_path(char* addr, char* host_name)
{
    string addr_str = addr;
    string host_name_str = host_name;
    string abs_path = "";

    size_t found = addr_str.find(host_name_str);
    if (found != string::npos)
    {
        for (size_t i = found + host_name_str.length(); i < addr_str.length(); i++)
            abs_path += addr_str[i];
    }

    if (abs_path == "/")
        return "/index.html";

    if (abs_path != "")
        return abs_path;
    
    return "/index.html";
}

string recvALineFromServerRepsonse(SOCKET sock_Connect, vector<string> &headers)
{
    int byte_recv = 0;
    string line = "";
    int line_length = 0;
    char recvbuff[1] = "";

    while (int(recvbuff[0]) == 13 || int(recvbuff[0]) == 10)
        byte_recv = recv(sock_Connect, recvbuff, 1, 0);
    
    while (true)
    {
        byte_recv = recv(sock_Connect, recvbuff, 1, 0);

        if (byte_recv > 0)
        {
            line += recvbuff;
            line_length++;
        }
            
        if ((line_length > 1) && (int(line[line_length - 2]) == 13) && (int(line[line_length - 1]) == 10)) //13: CR, 10: LF - '\r\n' in ASCII
        {
            headers.push_back(line);
            return line;
        }
    }
    
    return line;
}

void getStatusCodeInfo(string line, int &status_code)
{
    int i = 0, j;
    int n = line.length();
    while ((line[i] != ' ') && (i + 1 < n))
        i++;

    status_code = (int(line[i + 1]) - 48) * 100 + (int(line[i + 2]) - 48) * 10 + (int(line[i + 3]) - 48);
    cout << "Status: " << status_code << " " << getStatus(status_code) << "\n"; 
}

string getStatus(int status_code)
{
    //list of status code was taken directly from RFC 2616 about HTTP/1.1
    switch (status_code)
    {
        case 100:
            return "Continue";
            break;
        case 101:
            return "Switching Protocols";
            break;
        case 200:
            return "OK";
            break;
        case 201:
            return "Created";
            break;
        case 202:
            return "Accepted";
            break;
        case 203:
            return "Non-Authoritative Information";
            break;
        case 204:
            return "No Content";
            break;
        case 205:
            return "Reset Content";
            break;
        case 206:
            return "Partial Content";
            break;
        case 300:
            return "Multiple Choices";
            break;
        case 301:
            return "Moved Permanently";
            break;
        case 302:
            return "Found";
            break;
        case 303:
            return "See Other";
            break;
        case 304:
            return "Not Modified";
            break;
        case 305:
            return "Use Proxy";
            break;
        case 307:
            return "Temporary Redirect";
            break;
        case 400:
            return "Bad Request";
            break;
        case 401:
            return "Unauthorized";
            break;
        case 402:
            return "Payment Required";
            break;
        case 403:
            return "Forbidden";
            break;
        case 404:
            return "Not Found";
            break;
        case 405:
            return "Method Not Allowed";
            break;
        case 406:
            return "Not Acceptable";
            break;
        case 407:
            return "Proxy Authentication Required";
            break;
        case 408:
            return "Request Time-out";
            break;
        case 409:
            return "Conflict";
            break;
        case 410:
            return "Gone";
            break;
        case 411:
            return "Length Required";
            break;
        case 412:
            return "Precondition Failed";
            break;
        case 413:
            return "Request Entity Too Large";
            break;
        case 414:
            return "Request-URI Too Large";
            break;
        case 415:
            return "Unsupported Media Type";
            break;
        case 416:
            return "Requested range not satisfiable";
            break;
        case 417:
            return "Expectation Failed";
            break;
        case 500:
            return "Internal Server Error";
            break;
        case 501:
            return "Not Implemented";
            break;
        case 502:
            return "Bad Gateway";
            break;
        case 503:
            return "Service Unavailable";
            break;
        case 504:
            return "Gateway Time-out";
            break;
        case 505:
            return "HTTP Version not supported";
            break;
        default:
            return "Unknown extension-code";
            break;
    }
}

int getContentLength(string CL_header)
{
    int content_length = 0;
    int n = CL_header.length();

    for (int i = 0; i < n; i++)
        if ((int(CL_header[i]) >= 48) && (int(CL_header[i]) <= 57))
            content_length = content_length * 10 + (int(CL_header[i]) - 48);
    
    return content_length;
}

void downloadFile(SOCKET sock_Connect, string filepath, int content_length)
{
    ofstream fout;
    fout.open(filepath, ios::binary);

    if (fout.is_open())
    {
        int i = 0;
        int byte_recv;
        char recvbuff[1];
        while (i < content_length)
        {
            byte_recv = recv(sock_Connect, recvbuff, 1, 0);
            fout << recvbuff[0];

            i++;
        }

        cout << "Successfully downloaded file into " << filepath << ".\n";
    }
    else
        cout << "Cannot download file.\n";
}