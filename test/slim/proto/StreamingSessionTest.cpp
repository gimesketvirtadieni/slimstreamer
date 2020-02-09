/*
 * Copyright 2017, Andrej Kislovskij
 *
 * This is PUBLIC DOMAIN software so use at your own risk as it comes
 * with no warranties. This code is yours to share, use and modify without
 * any restrictions or obligations.
 *
 * For more information see conwrap/LICENSE or refer refer to http://unlicense.org
 *
 * Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)
 */

#include "slim/proto/StreamingSessionTest.hpp"


TEST(StreamingSessionTest, consumeChunk1)
{
    // TODO: work in progress
}

TEST_F(StreamingSessionFixture, getClientID1)
{
    EXPECT_EQ(type_safe::optional<std::string>{}, session.getClientID());

    auto id = std::string{"123"};
    session.setClientID(id);

    EXPECT_EQ(id, session.getClientID());
}

TEST_F(StreamingSessionFixture, getConnection1)
{
    EXPECT_EQ(&connection, &session.getConnection().get());
}

TEST(StreamingSessionTest, getFramesProvided1)
{
    // TODO: work in progress
}

TEST_F(StreamingSessionFixture, isRunning1)
{
    processor.process([&]
    {
        EXPECT_FALSE(session.isRunning());

        session.start();
        EXPECT_TRUE(session.isRunning());

        session.stop([&]
        {
            EXPECT_FALSE(session.isRunning());
        });
    });
}

TEST(StreamingSessionTest, onRequest1)
{
    // TODO: work in progress
}

TEST_F(StreamingSessionFixture, parseClientID1)
{
    auto str = "";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(str);
    EXPECT_FALSE(clientID.has_value());
}

TEST_F(StreamingSessionFixture, parseClientID2)
{
    auto str = "123";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(str);
    EXPECT_FALSE(clientID.has_value());
}

TEST_F(StreamingSessionFixture, parseClientID3)
{
    auto str = "123";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(std::string{"123="} + str);
    EXPECT_TRUE(clientID.has_value());
    type_safe::with(clientID, [&](const auto& clientID)
    {
        EXPECT_EQ(str, clientID);
    });
}

TEST_F(StreamingSessionFixture, parseClientID4)
{
    auto str = "123=123";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(std::string{"123="} + str);
    EXPECT_TRUE(clientID.has_value());
    type_safe::with(clientID, [&](const auto& clientID)
    {
        EXPECT_EQ(str, clientID);
    });
}

TEST_F(StreamingSessionFixture, start1)
{
    processor.process([&]
    {
        session.start();
        EXPECT_TRUE(session.isRunning());
        EXPECT_EQ(1, encoderPtr->startCalledTimes);

        std::stringstream ss;
        ss << "HTTP/1.1 200 OK\r\n"
           << "Server: SlimStreamer (" << VERSION << ")\r\n"
           << "Connection: close\r\n"
           << "Content-Type: audio/x-wave\r\n"
           << "\r\n";
        EXPECT_EQ(ss.str(), connection.writtenData.str());
    });
}

TEST_F(StreamingSessionFixture, stop1)
{
    auto p = std::promise<bool>{};
    auto f = p.get_future();

    processor.process([&, p = std::move(p)]() mutable
    {
        session.stop([p = std::move(p)]() mutable
        {
            p.set_value(true);
        });
    });

    auto status = f.wait_for(std::chrono::milliseconds(10));
    EXPECT_EQ(std::future_status::ready, status);
}

TEST_F(StreamingSessionFixture, stop2)
{
    auto p = std::promise<bool>{};
    auto f = p.get_future();

    processor.process([&, p = std::move(p)]() mutable
    {
        session.start();
        session.stop([p = std::move(p)]() mutable
        {
            p.set_value(true);
        });
    });

    auto status = f.wait_for(std::chrono::milliseconds(10));
    EXPECT_EQ(std::future_status::ready, status);
    if (status == std::future_status::ready)
    {
        EXPECT_EQ(1, encoderPtr->stopCalledTimes);
        EXPECT_EQ(1, connection.stopCalledTimes);

        // TODO: work in progress
        // validate buffer was flushed
    }
}
