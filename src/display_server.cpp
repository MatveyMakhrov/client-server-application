#include "../include/servers.hpp"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

DisplayServer::DisplayServer(int port)
	: serverPort(port), isRunning(false), serverSocket(-1) {

	#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
	}
	#endif
}

DisplayServer::~DisplayServer() {
	stop();
}

void DisplayServer::start() {
	serverSocket = createTCPSocket();
	if (serverSocket == -1) {
		std::cerr << "Failed to create TCP socket" << std::endl;
	}

	if (!bindTCPSocket(serverSocket, serverPort)) {
		std::cerr << "Failed to bind TCP socket to port " << serverPort << std::endl;
		closeTCPSocket(serverSocket);
		return;
	}

	if (!startTCPListening(serverSocket)) {
		std::cerr << "Failed to start TCP listening" << std::endl;
		closeTCPSocket(serverSocket);
		return;
	}

	isRunning = true;
	std::cout << "TCP Display Server started on port " << serverPort << std::endl;

	while (isRunning) {
		int clientSocket = acceptTCPConnection(serverSocket);
		if (clientSocket == -1) {
			if (isRunning) {
				std::cerr << "Failed to accept TCP connection" << std::endl;
			}
			continue;
		}

		handleClient(clientSocket);
		closeTCPSocket(clientSocket);
	}

	closeTCPSocket(serverSocket);

	#ifdef _WIN32
	WSACleanup();
	#endif
}

void DisplayServer::stop() {
	isRunning = false;
	#ifdef _WIN32
	shutdown(serverSocket, SD_BOTH);
	#else 
	shutdown(serverSocket, SD_RDWR);
	#endif
}

void DisplayServer::handleClient(int clientSocket) {
	while (isRunning) {
		try {
			uint32_t dataLength;
			int bytesReceived = receiveTCPData(clientSocket,
				reinterpret_cast<char*>(&dataLength), sizeof(dataLength));

			if (bytesReceived <= 0) break;

			dataLength = ntohl(dataLength);
			std::vector<char> buffer(dataLength + 1);

			bytesReceived = receiveTCPData(clientSocket, buffer.data(), dataLength);
			if (bytesReceived <= 0) break;

			buffer[bytesReceived] = '\0';
			std::cout << "Received: " << buffer.data() << std::endl;
		}
		catch (...) {
			break;
		}
	}
	closeTCPSocket(clientSocket);
}

int DisplayServer::createTCPSocket() {
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) {
		std::cerr << "Failed to create TCP socket" << std::endl;
	}
	return sock;
}

bool DisplayServer::bindTCPSocket(int socket, int port) {
	#ifdef _WIN32
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	#else
	sockaddr_in serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	#endif

	if (bind(socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		std::cerr << "Failed to bind TCP socket to port " << port << std::endl;
		return false;
	}
	return true;
}

bool DisplayServer::startTCPListening(int socket) {
	if (listen(socket, SOMAXCONN) < 0) {
		std::cerr << "Failed to start TCP listening" << std::endl;
		return false;
	}
	return true;
}

int DisplayServer::acceptTCPConnection(int socket) {
	#ifdef _WIN32
	sockaddr_in clientAddress;
	int clientAddressSize = sizeof(clientAddress);
	#else
	sockaddr_in clientAddress = {};
	socklen_t clientAddressSize = sizeof(clientAddress);
	#endif

	int clientSocket = accept(socket, (struct sockaddr*)&clientAddress, &clientAddressSize);
	if (clientSocket < 0) {
		std::cerr << "Failed to accept TCP connection" << std::endl;
	}
	return clientSocket;
}

int DisplayServer::receiveTCPData(int socket, char* buffer, size_t length) {
	#ifdef _WIN32
	return recv(socket, buffer, static_cast<int>(length), 0);
	#else
	return recv(socket, buffer, length, 0);
	#endif
}

void DisplayServer::closeTCPSocket(int socket) {
	if (socket != -1) {
		#ifdef _WIN32
		closesocket(socket);
		#else
		close(socket);
		#endif
	}
}