#include <iostream>
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

struct ClientInfo
{
    SOCKET socket;
    std::string name;
};

std::vector<ClientInfo> clients;
std::mutex clients_mutex;
std::mutex log_mutex;

void handleClient(SOCKET clientSocket)
{
    char buffer[1024];

    // Step 1: Receive client's name
    memset(buffer, 0, sizeof(buffer));
    int nameBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (nameBytes <= 0)
    {
        std::cout << "Failed to receive client name.\n";
        closesocket(clientSocket);
        return;
    }

    std::string clientName(buffer, nameBytes);
    bool isAdmin = (clientName == "admin");

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back({clientSocket, clientName});
    }

    std::cout << clientName << " connected.\n";

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cout << clientName << " disconnected.\n";
            break;
        }

        std::string message(buffer, bytesReceived);

        // Exit handling
        if (message == "exit" || message == "EXIT")
        {
            std::cout << clientName << " has exited the chat.\n";
            break;
        }

        // /list command
        if (message == "/list")
        {
            std::string clientList = "Connected clients:\n";
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto &client : clients)
                {
                    clientList += client.name + "\n";
                }
            }
            send(clientSocket, clientList.c_str(), clientList.size(), 0);
            continue;
        }

        // /msg command
        if (message.substr(0, 5) == "/msg ")
        {
            size_t spacePos = message.find(" ", 5);
            if (spacePos != std::string::npos)
            {
                std::string targetName = message.substr(5, spacePos - 5);
                std::string privateMessage = message.substr(spacePos + 1);
                std::string formatted = "[PM from " + clientName + "]: " + privateMessage;

                bool found = false;
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    for (const auto &client : clients)
                    {
                        if (client.name == targetName)
                        {
                            send(client.socket, formatted.c_str(), formatted.size(), 0);
                            found = true;
                            break;
                        }
                    }
                }

                if (!found)
                {
                    std::string errorMessage = "User " + targetName + " not found.\n";
                    send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
                }
            }
            continue;
        }

        // /kick command (only admin allowed)
        if (message.substr(0, 6) == "/kick ")
        {
            if (!isAdmin)
            {
                std::string noPerm = "You are not authorized to use /kick.\n";
                send(clientSocket, noPerm.c_str(), noPerm.size(), 0);
                continue;
            }

            std::string targetName = message.substr(6);
            bool kicked = false;

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (auto it = clients.begin(); it != clients.end(); ++it)
                {
                    if (it->name == targetName)
                    {
                        std::string kickMessage = "You have been kicked from the server.\n";
                        send(it->socket, kickMessage.c_str(), kickMessage.size(), 0);
                        closesocket(it->socket);
                        clients.erase(it);
                        std::cout << targetName << " has been kicked by admin.\n";
                        kicked = true;
                        break;
                    }
                }
            }

            if (!kicked)
            {
                std::string errorMessage = "User " + targetName + " not found.\n";
                send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
            }
            continue;
        }

        // Format message with name
        std::string fullMessage = clientName + ": " + message;
        std::cout << fullMessage << "\n";

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (const auto &client : clients)
            {
                if (client.socket != clientSocket)
                {
                    send(client.socket, fullMessage.c_str(), fullMessage.size(), 0);
                }
            }
        }

        // Log with client name
        {
            std::lock_guard<std::mutex> log_lock(log_mutex);
            std::ofstream logFile("chatlog.txt", std::ios::app);
            if (logFile.is_open())
            {
                logFile << fullMessage << std::endl;
            }
        }
    }

    // Remove client from list
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [clientSocket](const ClientInfo &ci)
                                     { return ci.socket == clientSocket; }),
                      clients.end());
    }

    closesocket(clientSocket);
}

int main()
{
    WSADATA wsaData;
    SOCKET server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 5) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true)
    {
        SOCKET clientSocket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Failed to accept client\n";
            continue;
        }

        std::cout << "New client connected!\n";
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(server_fd);
    WSACleanup();

    return 0;
}
