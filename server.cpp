// server.cpp (for Windows)
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // Link Winsock library

#define PORT 8080

using namespace std;

int main()
{
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Step 1: Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    // Step 2: Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // Step 3: Bind socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        cerr << "Bind failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Step 4: Listen
    if (listen(server_fd, 3) == SOCKET_ERROR)
    {
        cerr << "Listen failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    cout << "Server listening on port " << PORT << "...\n";

    // Step 5: Accept a client
    new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket == INVALID_SOCKET)
    {
        cerr << "Accept failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    cout << "Client connected!\n";

    // Step 6: Receive message from client
    char buffer[1024] = {0};
    int bytesReceived = recv(new_socket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        cerr << "Failed to receive data\n";
    }
    else
    {
        cout << "Client says: " << buffer << "\n";
    }

    // step 7: Clean up section
    closesocket(new_socket);
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
