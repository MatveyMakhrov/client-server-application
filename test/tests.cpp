#include "../include/client.hpp"
#include "../include/servers.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstdlib>

const int TEST_DISPLAY_PORT = 7071;
const int TEST_PROCESSING_PORT = 9091;
const std::string TEST_HOST = "127.0.0.1";

class ServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        display_server_thread = std::thread([] {
            DisplayServer server(TEST_DISPLAY_PORT);
            server.start();
            });

        processing_server_thread = std::thread([] {
            ProcessingServer server(TEST_PROCESSING_PORT, TEST_HOST, TEST_DISPLAY_PORT);
            server.start();
            });

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        system("taskkill /F /IM tests.exe");
        display_server_thread.join();
        processing_server_thread.join();
    }

    std::thread display_server_thread;
    std::thread processing_server_thread;
};

// ���� 1: �������� ����������� ������� � ������� ���������
TEST_F(ServerTest, ClientConnectsToProcessingServer) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    EXPECT_TRUE(client.connectToServer());
}

// ���� 2: �������� ��������� ������ � �������� ����������
TEST(ProcessingServerTest, ProcessesDataAndRemovesDuplicates) {
    ProcessingServer server(0, "", 0); // ���� 0 ��� �����

    std::string testData = "hello world hello test";
    std::string expected = "hello world test";
    std::string result = server.processData(testData);

    EXPECT_EQ(result, expected);
}

// ���� 3: �������� ��������� ������
TEST(ProcessingServerTest, ValidatesDataCorrectly) {
    ProcessingServer server(0, "", 0);

    EXPECT_FALSE(server.validateData(""));
    EXPECT_TRUE(server.validateData("valid data"));
}

// ���� 4: �������� �������� � ��������� ������
TEST_F(ServerTest, ClientSendsAndReceivesData) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    ASSERT_TRUE(client.connectToServer());

    EXPECT_TRUE(client.sendData("test message"));
    EXPECT_TRUE(client.receiveAcknowledgement());
}

// ���� 5: �������� ������ ���� �������
TEST_F(ServerTest, FullChainTest) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    ASSERT_TRUE(client.connectToServer());

    // ������ ���������
    EXPECT_TRUE(client.sendData("hello world hello"));
    EXPECT_TRUE(client.receiveAcknowledgement());

    // ������ ���������
    EXPECT_TRUE(client.sendData("test test message"));
    EXPECT_TRUE(client.receiveAcknowledgement());
}

// ���� 6: �������� ��������� �������� ������
TEST_F(ServerTest, InvalidDataHandling) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    ASSERT_TRUE(client.connectToServer());

    EXPECT_FALSE(client.sendData(""));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}