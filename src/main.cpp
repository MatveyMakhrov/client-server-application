#include "../include/client.hpp"
#include "../include/servers.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <csignal>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#endif

std::atomic<bool> isRunning(true);

void setupSignalHandlers() {
    #ifdef _WIN32
    SetConsoleCtrlHandler([](DWORD type) {
        if (type == CTRL_C_EVENT) {
            isRunning = false;
            return TRUE;
        }
        return FALSE;
        }, TRUE);
    #else
    signal(SIGINT, [](int) { isRunning = false; });
    #endif
}

void runDisplayServer(int port) {
    while (isRunning) {
        try {
            DisplayServer server(port);
            std::cout << "Display Server started on port " << port << std::endl;
            server.start();
        }
        catch (const std::exception& e) {
            std::cerr << "Display Server error: " << e.what() << std::endl;
            std::cerr << "Restarting Display Server in 3 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}

void runProcessingServer(int port, const std::string& displayHost, int displayPort) {
    while (isRunning) {
        try {
            ProcessingServer server(port, displayHost, displayPort);
            std::cout << "Processing Server started on port " << port
                << ", connected to display server at " << displayHost
                << ":" << displayPort << std::endl;
            server.start();
        }
        catch (const std::exception& e) {
            std::cerr << "Processing Server error: " << e.what() << std::endl;
            std::cerr << "Restarting Processing Server in 3 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}

void runClient(const std::string& host, int port) {
    try {
        Client client(host, port);
        std::cout << "Client connected to " << host << ":" << port << std::endl;
        std::cout << "Enter messages (type 'exit' to quit):" << std::endl;
        client.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

void printUsage() {
    std::cout << "Client-Server Application\n\n";
    std::cout << "Usage:\n";
    std::cout << "  To run Display Server:    ./app display <port>\n";
    std::cout << "  To run Processing Server: ./app processing <port> <display_host> <display_port>\n";
    std::cout << "  To run Client:            ./app client <server_host> <server_port>\n";
    std::cout << "  To run all components:    ./app all <client_port> <processing_port> <display_port>\n\n";
    std::cout << "Example:\n";
    std::cout << "  ./app all 8080 9090 7070\n";
}

int main(int argc, char* argv[]) {
    setupSignalHandlers();

    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string mode = argv[1];

    try {
        if (mode == "display" && argc == 3) {
            int port = std::stoi(argv[2]);
            runDisplayServer(port);
        }
        else if (mode == "processing" && argc == 5) {
            int port = std::stoi(argv[2]);
            std::string displayHost = argv[3];
            int displayPort = std::stoi(argv[4]);
            runProcessingServer(port, displayHost, displayPort);
        }
        else if (mode == "client" && argc == 4) {
            std::string host = argv[2];
            int port = std::stoi(argv[3]);
            runClient(host, port);
        }
        else if (mode == "all" && argc == 5) {
            int clientPort = std::stoi(argv[2]);
            int processingPort = std::stoi(argv[3]);
            int displayPort = std::stoi(argv[4]);

            std::thread displayThread(runDisplayServer, displayPort);
            displayThread.detach();

            std::this_thread::sleep_for(std::chrono::seconds(3));

            std::thread processingThread(runProcessingServer,
                processingPort, "127.0.0.1", displayPort);
            processingThread.detach();

            std::this_thread::sleep_for(std::chrono::seconds(3));

            runClient("127.0.0.1", processingPort);
        }
        else {
            printUsage();
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}