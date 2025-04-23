#include "../include/servers.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
using ssize_t = int;
#endif

ProcessingServer::ProcessingServer(int port, const std::string& displayHost, int displayPort)
	: serverPort(port), displayServerHost(displayHost),
	displayServerPort(displayPort), isRunning(false),
	serverSocket(-1), displayServerSocket(-1) {

	#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
	}
	#endif
}

ProcessingServer::~ProcessingServer() {
	stop();
}

void ProcessingServer::start() {
	serverSocket = createTCPSocket();
	if (serverSocket < 0) {
		std::cerr << "Failed to create server TCP socket" << std::endl;
		return;
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

	if (!connectToDisplayServer()) {
		std::cerr << "Failed to establish TCP connection to display server" << std::endl;
		closeTCPSocket(serverSocket);
		return;
	}

	isRunning = true;
	std::cout << "TCP Processing server started on port " << serverPort << std::endl;
	std::cout << "TCP Connected to display server at " << displayServerHost
		<< ":" << displayServerPort << std::endl;

	while (isRunning) {
		int clientSocket = acceptTCPConnection(serverSocket);
		if (clientSocket < 0) {
			if (isRunning) {
				std::cerr << "Accept error, retrying..." << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			continue;
		}

		std::thread([this, clientSocket]() {
			while (true) {
				if (!handleClient(clientSocket)) {
					break;
				}
			}
			closeTCPSocket(clientSocket);
			}).detach();
	}

	closeTCPSocket(serverSocket);
	closeTCPSocket(displayServerSocket);

	#ifdef _WIN32
	WSACleanup();
	#endif
}

void ProcessingServer::stop() {
	isRunning = false;
	#ifdef _WIN32
	shutdown(serverSocket, SD_BOTH);
	#else
	shutdown(serverSocket, SHUT_RDWR);
	#endif
}

bool ProcessingServer::handleClient(int clientSocket) {
	std::cout << "[DEBUG] Handling new client connection" << std::endl;
	while (isRunning) {  // Добавляем цикл для обработки нескольких сообщений
		try {
			const int BUFFER_SIZE = 4096;
			char buffer[BUFFER_SIZE];

			// Получаем длину данных
			uint32_t dataLength;
			int bytesReceived = receiveTCPData(clientSocket,
				reinterpret_cast<char*>(&dataLength), sizeof(dataLength));

			if (bytesReceived <= 0) break;

			dataLength = ntohl(dataLength);
			if (dataLength == 0 || dataLength > BUFFER_SIZE - 1) {
				std::cerr << "Invalid data length" << std::endl;
				continue;
			}

			// Получаем данные
			bytesReceived = receiveTCPData(clientSocket, buffer, dataLength);
			if (bytesReceived <= 0) break;

			buffer[bytesReceived] = '\0';
			std::string processedData = processData(buffer);

			// Отправка с повторными попытками
			for (int i = 0; i < 3; i++) {
				if (sendToDisplayServer(processedData) &&
					sendAcknowledgement(clientSocket)) {
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		catch (...) {
			return false;
		}
	}
	return true;
}

bool ProcessingServer::sendToDisplayServer(const std::string& processedData) {
	if (displayServerSocket == -1) {
		std::cerr << "Not connected to display server" << std::endl;
		return false;
	}

	uint32_t dataLength = static_cast<uint32_t>(processedData.size());
	uint32_t networkLength = htonl(dataLength);
	if (sendTCPData(displayServerSocket, reinterpret_cast<const char*>(&networkLength), sizeof(networkLength)) <= 0) {
		std::cerr << "Failed to send data length to display server" << std::endl;
		return false;
	}

	if (sendTCPData(displayServerSocket, processedData.c_str(), processedData.size()) <= 0) {
		std::cerr << "Failed to send data to display server" << std::endl;
		return false;
	}

	return true;
}

bool ProcessingServer::connectToDisplayServer() {
	displayServerSocket = createTCPSocket();
	if (displayServerSocket == -1) {
		return false;
	}

	#ifdef _WIN32
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(displayServerPort);
	serverAddress.sin_addr.s_addr = inet_addr(displayServerHost.c_str());
	#else
	sockaddr_in serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(displayServerPort);
	inet_pton(AF_INET, displayServerHost.c_str(), &serverAddress.sin_addr);
	#endif

	if (connect(displayServerSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
	#ifdef _WIN32
		std::cerr << "Connection failed. Error: " << WSAGetLastError() << std::endl;
	#else
		std::cerr << "Connection failed. Error: " << strerror(errno) << std::endl;
	#endif
		closeTCPSocket(displayServerSocket);
		displayServerSocket = -1;
		return false;
	}
	return true;
}

bool ProcessingServer::validateData(const std::string& data) {
	return !data.empty();
}

std::string ProcessingServer::processData(const std::string& data) {
	std::istringstream iss(data);
	std::unordered_set<std::string> uniqueWords;
	std::string word;

	while (iss >> word) {
		uniqueWords.insert(word);
	}

	std::string result;
	for (const auto& w : uniqueWords) {
		if (!result.empty()) {
			result += " ";
		}
		result += w;
	}

	return result;
}


bool ProcessingServer::sendAcknowledgement(int clientSocket) {
	const std::string ack = "OK";
	return sendTCPData(clientSocket, ack.c_str(), ack.size()) > 0;
}

int ProcessingServer::createTCPSocket() {
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) {
		std::cerr << "Failed to create TCP socket" << std::endl;
	}
	return sock;
}

bool ProcessingServer::bindTCPSocket(int socket, int port) {
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

	return bind(socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0;
}

bool ProcessingServer::startTCPListening(int socket) {
	return listen(socket, SOMAXCONN) == 0;
}

int ProcessingServer::acceptTCPConnection(int socket) {
	#ifdef _WIN32
	sockaddr_in clientAddress;
	int clientAddressSize = sizeof(clientAddress);
	#else
	sockaddr_in clientAddress = {};
	socklen_t clientAddressSize = sizeof(clientAddress);
	#endif

	return accept(socket, (struct sockaddr*)&clientAddress, &clientAddressSize);
}

int ProcessingServer::receiveTCPData(int socket, char* buffer, size_t length) {
	#ifdef _WIN32
	return recv(socket, buffer, length, 0);
	#else
	return recv(socket, buffer, length, 0);
	#endif
}


int ProcessingServer::sendTCPData(int socket, const char* data, size_t length) {
	#ifdef _WIN32
	return send(socket, data, length, 0);
	#else
	return send(socket, data, length, 0);
	#endif
}

void ProcessingServer::closeTCPSocket(int socket) {
	if (socket != -1) {
		#ifdef _WIN32
		closesocket(socket);
		#else
		close(socket);
		#endif
	}
}

