#include "../include/client.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

Client::Client(const std::string& serverHost, int serverPort)
    : serverHost(serverHost), serverPort(serverPort),
    isRunning(false), clientSocket(-1) {

    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
    }
    #endif
}

Client::~Client() {
    disconnect();
}

bool Client::connectToServer() {
    clientSocket = createTCPSocket();
    if (clientSocket == -1) {
        std::cerr << "Failed to create TCP socket" << std::endl;
        return false;
    }

    if (!connectTCPSocket(clientSocket, serverHost, serverPort)) {
        std::cerr << "Failed to connect to server " << serverHost
            << ":" << serverPort << std::endl;
        closeTCPSocket(clientSocket);
        clientSocket = -1;
        return false;
    }

    std::cout << "Connected to processing server at " << serverHost
        << ":" << serverPort << std::endl;
    std::cout << "Client connected to " << std::endl;
    return true;
}

void Client::run() {
    if (clientSocket == -1 && !connectToServer()) {
        return;
    }

    isRunning = true;
    std::cout << "Enter messages to send to server (type 'exit' to quit):" << std::endl;

    std::string input;
    while (isRunning) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "exit") {
            disconnect();
            break;
        }

        if (!sendData(input)) {
            std::cerr << "Failed to send data to server. Reconnecting..." << std::endl;
            if (!connectToServer()) {
                break;
            }
            continue;
        }

        if (!receiveAcknowledgement()) {
            std::cerr << "No acknowledgement received. Reconnecting..." << std::endl;
            if (!connectToServer()) {
                break;
            }
        }
    }
}

void Client::disconnect() {
    isRunning = false;
    if (clientSocket != -1) {
    #ifdef _WIN32
        shutdown(clientSocket, SD_BOTH);
    #else
        shutdown(clientSocket, SHUT_RDWR);
    #endif
        closeTCPSocket(clientSocket);
        clientSocket = -1;
    }

    #ifdef _WIN32
    WSACleanup();
    #endif
}

bool Client::sendData(const std::string& data) {
    for (int attempt = 0; attempt < 3; attempt++) {
        if (clientSocket == -1 && !connectToServer()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        uint32_t dataLength = static_cast<uint32_t>(data.size());
        uint32_t networkLength = htonl(dataLength);

        if (sendTCPData(clientSocket, reinterpret_cast<const char*>(&networkLength),
            sizeof(networkLength)) <= 0 ||
            sendTCPData(clientSocket, data.c_str(), data.size()) <= 0) {
            disconnect();
            continue;
        }

        return receiveAcknowledgement();
    }
    return false;
}

bool Client::receiveAcknowledgement() {
    const int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];

    int bytesReceived = receiveTCPData(clientSocket, buffer, BUFFER_SIZE - 1);
    if (bytesReceived <= 0) {
        return false;
    }

    buffer[bytesReceived] = '\0';
    return std::string(buffer) == "OK";
}


int Client::createTCPSocket() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        std::cerr << "Failed to create TCP socket" << std::endl;
    }
    return sock;
}

bool Client::connectTCPSocket(int socket, const std::string& host, int port) {
    #ifdef _WIN32
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(host.c_str());
    #else
    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr);
    #endif

    if (connect(socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        #ifdef _WIN32
        std::cerr << "Failed to connect to " << host << ":" << port
            << " - Error: " << WSAGetLastError() << std::endl;
        #else
        std::cerr << "Failed to connect to " << host << ":" << port
            << " - Error: " << strerror(errno) << std::endl;
        #endif
        return false;
    }
    return true;
}

int Client::sendTCPData(int socket, const char* data, size_t length) {
    #ifdef _WIN32
    return send(socket, data, static_cast<int>(length), 0);
    #else
    return send(socket, data, length, 0);
    #endif
}

int Client::receiveTCPData(int socket, char* buffer, size_t length) {
    #ifdef _WIN32
    return recv(socket, buffer, static_cast<int>(length), 0);
    #else
    return recv(socket, buffer, length, 0);
    #endif
}

void Client::closeTCPSocket(int socket) {
    if (socket != -1) {
    #ifdef _WIN32
        closesocket(socket);
    #else
        close(socket);
    #endif
    }
}