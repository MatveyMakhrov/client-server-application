#include <string>
#include <unordered_set>
#include <vector>
#include <cerrno>
#include <atomic>

class ProcessingServer {
public:
	ProcessingServer(int port, const std::string& displayHost, int displayPort);
	~ProcessingServer();

	void start();
	void stop();
	std::string processData(const std::string& data);
	bool validateData(const std::string& data);

private:
	int serverPort;
	std::string displayServerHost;
	int displayServerPort;
	bool isRunning;
	int serverSocket;
	int displayServerSocket;

	bool handleClient(int clientSocket);
	bool connectToDisplayServer();
	bool sendToDisplayServer(const std::string& processedData);
	bool sendAcknowledgement(int clientSocket);

	int createTCPSocket();
	bool bindTCPSocket(int socket, int port);
	bool startTCPListening(int socket);
	int acceptTCPConnection(int socket);
	int receiveTCPData(int socket, char* buffer, size_t length);
	int sendTCPData(int socket, const char* data, size_t lenght);
	void closeTCPSocket(int socket);
};


class DisplayServer {
public:
	explicit DisplayServer(int port);
	~DisplayServer();

	void start();
	void stop();

private:
	int serverPort;
	std::atomic<bool> isRunning;
	int serverSocket;

	void handleClient(int clientSocket);

	int createTCPSocket();
	bool bindTCPSocket(int socket, int port);
	bool startTCPListening(int socket);
	int acceptTCPConnection(int socket);
	int receiveTCPData(int socket, char* buffer, size_t lenght);
	void closeTCPSocket(int socket);
};