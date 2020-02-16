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

std::atomic<unsigned int> StreamingSessionFixture::invocationsCounter{0};

TEST(StreamingSessionTest, consumeChunk1)
{
    // TODO: work in progress
}

TEST_F(StreamingSessionFixture, getConnection1)
{
    ASSERT_EQ(&connection, &session.getConnection().get());
}

TEST(StreamingSessionTest, getFramesProvided1)
{
    // TODO: work in progress
}

TEST_F(StreamingSessionFixture, isRunning1)
{
    processor.process([&]
    {
        ASSERT_FALSE(session.isRunning());

        session.start();
        ASSERT_TRUE(session.isRunning());

        session.stop([&]
        {
            ASSERT_FALSE(session.isRunning());
        });
    });
}

TEST_F(StreamingSessionFixture, onRequest1)
{
    processor.process([&]
    {
        auto str = std::string{""};
        session.onRequest((const unsigned char*)str.c_str(), str.length());

        ASSERT_FALSE(session.getClientID().has_value());
    });
}

TEST_F(StreamingSessionFixture, onRequest2)
{
    processor.process([&]
    {
        auto str = std::string{"123"};
        session.onRequest((const unsigned char*)str.c_str(), str.length());

        ASSERT_FALSE(session.getClientID().has_value());
    });
}

TEST_F(StreamingSessionFixture, onRequest3)
{
    processor.process([&]
    {
        auto id  = std::string{"123"};
        auto str = std::string{"aaa="} + id;

        session.onRequest((const unsigned char*)str.c_str(), str.length());

        ASSERT_TRUE(session.getClientID().has_value());
        ASSERT_EQ(id, session.getClientID().value());
    });
}

TEST_F(StreamingSessionFixture, onRequest4)
{
    processor.process([&]
    {
        auto id  = std::string{"123=123"};
        auto str = std::string{"aaa="} + id;

        session.onRequest((const unsigned char*)str.c_str(), str.length());

        ASSERT_TRUE(session.getClientID().has_value());
        ASSERT_EQ(id, session.getClientID().value());
    });
}

TEST_F(StreamingSessionFixture, start1)
{
    processor.process([&]
    {
        session.start();
        ASSERT_TRUE(session.isRunning());
        ASSERT_EQ(1, encoderPtr->startCalledTimes);

        std::stringstream ss;
        ss << "HTTP/1.1 200 OK\r\n"
           << "Server: SlimStreamer (" << VERSION << ")\r\n"
           << "Connection: close\r\n"
           << "Content-Type: audio/x-wave\r\n"
           << "\r\n";
        ASSERT_EQ(ss.str(), connection.writtenData.str());
    });
}

TEST_F(StreamingSessionFixture, stop1)
{
    std::atomic<unsigned int> sessionStopCalledTimes{0};

    ASSERT_EQ(std::future_status::ready, processAndWait([&]
    {
        session.stop([&]
        {
            sessionStopCalledTimes++;
        });
    }));
    ASSERT_EQ(0, encoderPtr->stopCalledTimes);
    ASSERT_EQ(0, connection.stopCalledTimes);
    ASSERT_EQ(1, sessionStopCalledTimes);
}

TEST_F(StreamingSessionFixture, stop2)
{
    unsigned char             data[] = {'1', '2', '3'};
    std::atomic<unsigned int> sessionStopCalledTimes{0};
    std::atomic<unsigned int> sessionStopCalledSequence;

    ASSERT_EQ(std::future_status::ready, processAndWait([&]
    {
        session.start();
        connection.writtenData.str("");
        connection.writtenData.clear();

        session.submitData(data, sizeof(data));
        session.stop([&]
        {
            sessionStopCalledTimes++;
            sessionStopCalledSequence = ++invocationsCounter;
        });
    }));
    ASSERT_EQ(1, encoderPtr->stopCalledTimes);
    ASSERT_EQ(0, connection.stopCalledTimes);
    ASSERT_EQ(0, sessionStopCalledTimes);

    ASSERT_EQ(std::future_status::ready, processAndWait([&]
    {
        connection.flush();
    }));
    ASSERT_EQ(1, encoderPtr->stopCalledTimes);
    ASSERT_EQ(1, connection.stopCalledTimes);
    ASSERT_EQ(1, sessionStopCalledTimes);

    ASSERT_TRUE(encoderPtr->stopCalledSequence < connection.stopCalledSequence);
    ASSERT_TRUE(connection.stopCalledSequence < sessionStopCalledSequence);
}
