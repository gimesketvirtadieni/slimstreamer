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

TEST_F(StreamingSessionFixture, getClientID1)
{
    ASSERT_EQ(type_safe::optional<std::string>{}, session.getClientID());

    auto id = std::string{"123"};
    session.setClientID(id);

    ASSERT_EQ(id, session.getClientID());
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

TEST(StreamingSessionTest, onRequest1)
{
    // TODO: work in progress
}

TEST_F(StreamingSessionFixture, parseClientID1)
{
    auto str = "";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(str);

    ASSERT_FALSE(clientID.has_value());
}

TEST_F(StreamingSessionFixture, parseClientID2)
{
    auto str = "123";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(str);

    ASSERT_FALSE(clientID.has_value());
}

TEST_F(StreamingSessionFixture, parseClientID3)
{
    auto str = "123";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(std::string{"123="} + str);

    ASSERT_EQ(str, clientID.value_or("456"));
}

TEST_F(StreamingSessionFixture, parseClientID4)
{
    auto str = "123=123";
    auto clientID = StreamingSessionFixture::StreamingSessionTest::parseClientID(std::string{"123="} + str);

    ASSERT_EQ(str, clientID.value_or("456"));
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
