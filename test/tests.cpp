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

// Тест 1: Проверка подключения клиента к серверу обработки
TEST_F(ServerTest, ClientConnectsToProcessingServer) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    EXPECT_TRUE(client.connectToServer());
}

// Тест 2: Проверка обработки данных и удаления дубликатов
TEST(ProcessingServerTest, ProcessesDataAndRemovesDuplicates) {
    ProcessingServer server(0, "", 0); // Порт 0 для теста

    std::string testData = "hello world hello test";
    std::string expected = "hello world test";
    std::string result = server.processData(testData);

    EXPECT_EQ(result, expected);
}

// Тест 3: Проверка валидации данных
TEST(ProcessingServerTest, ValidatesDataCorrectly) {
    ProcessingServer server(0, "", 0);

    EXPECT_FALSE(server.validateData(""));
    EXPECT_TRUE(server.validateData("valid data"));
}

// Тест 4: Проверка отправки и получения данных
TEST_F(ServerTest, ClientSendsAndReceivesData) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    ASSERT_TRUE(client.connectToServer());

    EXPECT_TRUE(client.sendData("test message"));
    EXPECT_TRUE(client.receiveAcknowledgement());
}

// Тест 5: Проверка работы всей цепочки
TEST_F(ServerTest, FullChainTest) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    ASSERT_TRUE(client.connectToServer());

    // Первое сообщение
    EXPECT_TRUE(client.sendData("hello world hello"));
    EXPECT_TRUE(client.receiveAcknowledgement());

    // Второе сообщение
    EXPECT_TRUE(client.sendData("test test message"));
    EXPECT_TRUE(client.receiveAcknowledgement());
}

// Тест 6: Проверка обработки неверных данных
TEST_F(ServerTest, InvalidDataHandling) {
    Client client(TEST_HOST, TEST_PROCESSING_PORT);
    ASSERT_TRUE(client.connectToServer());

    EXPECT_FALSE(client.sendData(""));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}