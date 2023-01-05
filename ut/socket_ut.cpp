#include "tcp_server.h"

#include <clickhouse/base/socket.h>
#include <gtest/gtest.h>

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <thread>

using namespace clickhouse;

TEST(Socketcase, connecterror) {
    int port = 19978;
    NetworkAddress addr("localhost", std::to_string(port));
    LocalTcpServer server(port);
    server.start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    try {
        Socket socket(addr);
    } catch (const std::system_error& e) {
        FAIL();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.stop();
    try {
        Socket socket(addr);
        FAIL();
    } catch (const std::system_error& e) {
        ASSERT_NE(EINPROGRESS,e.code().value());
    }
}

TEST(Socketcase, timeoutrecv) {
    using Seconds = std::chrono::seconds;

    int port = 19979;
    NetworkAddress addr("localhost", std::to_string(port));
    LocalTcpServer server(port);
    server.start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    try {
        Socket socket(addr, SocketTimeoutParams { Seconds(5), Seconds(5), Seconds(5) });

        std::unique_ptr<InputStream> ptr_input_stream = socket.makeInputStream();
        char buf[1024];
        ptr_input_stream->Read(buf, sizeof(buf));

    }
    catch (const std::system_error& e) {
#if defined(_unix_)
        auto expected = EAGAIN;
#else
        auto expected = WSAETIMEDOUT;
#endif
        ASSERT_EQ(expected, e.code().value());
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.stop();
}

TEST(Socketcase, connecttimeout) {
    using Clock = std::chrono::steady_clock;

    try {
        NetworkAddress("::1", "19980");
    } catch (const std::system_error& e) {
        GTEST_SKIP() << "missing IPv6 support";
    }

    NetworkAddress addr("100::1", "19980");  // "discard" IPv6 address

    const auto connect_start = Clock::now();
    try {
        Socket socket(addr, SocketTimeoutParams{std::chrono::milliseconds(100)});
        FAIL();
    } catch (const std::system_error& e) {
        if (e.code().value() == ENETUNREACH) {
            GTEST_SKIP() << "missing IPv6 support";
        }
        EXPECT_EQ(EINPROGRESS, e.code().value());
        EXPECT_LT(Clock::now() - connect_start, std::chrono::seconds(5));
    }
}

// Test to verify that reading from empty socket doesn't hangs.
//TEST(Socketcase, ReadFromEmptySocket) {
//    const int port = 12345;
//    const NetworkAddress addr("127.0.0.1", std::to_string(port));

//    LocalTcpServer server(port);
//    server.start();

//    std::this_thread::sleep_for(std::chrono::seconds(1));

//    char buffer[1024];
//    Socket socket(addr);
//    socket.SetTcpNoDelay(true);
//    auto input = socket.makeInputStream();
//    input->Read(buffer, sizeof(buffer));
//}
