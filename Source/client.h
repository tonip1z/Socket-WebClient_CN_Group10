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

//main processing function
void process_address(char* addr, bool multi_threaded);
bool REQUEST_QUERY(SOCKET sock_Connect, char* addr, char* host_name, bool multi_threaded);
bool REQUEST_QUERY_FILENAME(SOCKET sock_Connect, char* host_name, string abs_path, string file_name, bool multi_threaded);
void RESPONSE_QUERY(SOCKET sock_Connect, char* addr, char* host_name, bool multi_threaded, string folder_dir);
bool RESPONSE_QUERY_GET_FILENAMES(SOCKET sock_Connect, char* addr, char* host_name, bool multi_threaded, vector<string> &file_names);
void RESPONSE_QUERY_FILENAME(SOCKET sock_Connect, char* addr, char* host_name, string file_name, bool multi_threaded, string folder_dir);

//support functions
char* getHostnameFromURL(char* URL);
bool is_HTTP_URL(char* host_name);
string getIPv4(sockaddr* addr);
string create_GET_query(char* addr, char* host_name);
string get_abs_path(char* addr, char* host_name);
bool hasFolderName(string abs_path);
string getFolderName(string abs_path);
bool isFileName(string filename);
string recvALineFromServerRepsonse(SOCKET sock_Connect, vector<string> &lines);
void getStatusCodeInfo(string line, int &status_code);
string getStatus(int status_code);
int getContentLength(string CL_header);
string get_filename(char* addr);
int getChunkSize(string chunk_size_16);
void readChunk(ofstream &fout, SOCKET sock_Connect, int chunk_size);
bool readCRLF(SOCKET sock_Connect);
void downloadFile(SOCKET sock_Connect, string filename, int content_length, bool multi_threaded, string folder_dir);
string progressBar(float progress);
void printline(string line);