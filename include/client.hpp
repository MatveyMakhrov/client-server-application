#include <string>
#include <atomic>

class Client {
public:
	Client(const std::string& serverHost, int serverPort);
	~Client();

	bool connectToServer();
	void run();
	void disconnect();
	bool sendData(const std::string& data);
	bool receiveAcknowledgement();

private:
	std::string serverHost;
	int serverPort;
	std::atomic<bool> isRunning;
	int clientSocket;

	int createTCPSocket();
	bool connectTCPSocket(int socket, const std::string& host, int port);
	int sendTCPData(int socket, const char* data, size_t length);
	int receiveTCPData(int socket, char* buffer, size_t length);
	void closeTCPSocket(int socket);
};
