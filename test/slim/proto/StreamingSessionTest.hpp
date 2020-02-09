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

#include <conwrap2/Processor.hpp>
#include <conwrap2/ProcessorProxy.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <ofats/invocable.h>


#include "slim/ContainerBase.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/proto/StreamingSession.hpp"

struct ConnectionMock
{
    void stop()
    {
        stopCalledTimes++;
    }

    template<class CallbackType>
    void writeAsync(const void* data, const std::size_t size, CallbackType callback = [](auto, auto) {}) {}

    void write(std::string str)
    {
        writtenData << str;
    }

    std::size_t write(const void* data, const std::size_t size)
    {
        return size;
    }

    unsigned int      stopCalledTimes{0};
    std::stringstream writtenData;
};

struct ContainerMock : public slim::ContainerBase
{
    ContainerMock(conwrap2::ProcessorProxy<std::unique_ptr<slim::ContainerBase>> pp)
    : processorProxy{pp} {}

    virtual bool isSchedulerRunning() override
    {
        return true;
    };

    virtual void start() override {}
    virtual void stop() override {}

    conwrap2::ProcessorProxy<std::unique_ptr<slim::ContainerBase>> processorProxy;
};

struct EncoderMock : public slim::EncoderBase
{
    EncoderMock(unsigned int ch, unsigned int bs, unsigned int bv, unsigned int sr, std::string ex, std::string mm, EncodedCallbackType ec)
    : EncoderBase{ch, bs, bv, sr, ex, mm, ec} {}

    virtual void encode(unsigned char* data, const std::size_t size) override {}
    virtual bool isRunning() {return true;}
    virtual void start()
    {
        startCalledTimes++;
    }

    virtual void stop(ofats::any_invocable<void()> callback)
    {
        stopCalledTimes++;
        callback();
    }

    unsigned int startCalledTimes{0};
    unsigned int stopCalledTimes{0};
};

struct StreamingSessionFixture : public ::testing::TestWithParam<std::size_t>
{
    using Processor            = conwrap2::Processor<std::unique_ptr<slim::ContainerBase>>;
    using Connection           = ConnectionMock;
    using EncoderBuilder       = slim::EncoderBuilder;
    using StreamingSessionTest = slim::proto::StreamingSession<ConnectionMock>;

    ~StreamingSessionFixture()
    {
        // processor must be destroyed FIRST as it may contain references to other object
        processorPtr.reset();
    }

    EncoderBuilder createEncoderBuilder()
    {
        EncoderBuilder encoderBuilder;

        encoderBuilder.setHeader(false);
        encoderBuilder.setFormat(slim::proto::FormatSelection::PCM);
        encoderBuilder.setExtention("wav");
        encoderBuilder.setMIME("audio/x-wave");
        encoderBuilder.setSamplingRate(44100);
        encoderBuilder.setChannels(2);
        encoderBuilder.setBitsPerSample(32);
        encoderBuilder.setBitsPerValue(24);
        encoderBuilder.setBuilder([&](unsigned int ch, unsigned int bs, unsigned int bv, unsigned int sr, bool hd, std::string ex, std::string mm, std::function<void(unsigned char*, std::size_t)> ec)
        {
            // saving a pointer to the encoder in the fixture so interaction can be examined
            encoderPtr = new EncoderMock{ch, bs, bv, sr, ex, mm, ec};
            return std::unique_ptr<slim::EncoderBase>{encoderPtr};
        });

        return encoderBuilder;
    }

    static Processor createProcessor()
    {
        return Processor
        {
            [&](auto processorProxy)
            {
                return std::unique_ptr<slim::ContainerBase>
                {
                    new ContainerMock{processorProxy}
                };
            }
        };
    }

    std::unique_ptr<Processor> processorPtr{std::make_unique<Processor>([&](auto processorProxy)
    {
        return std::unique_ptr<slim::ContainerBase>
        {
            new ContainerMock{processorProxy}
        };
    })};
    Processor&           processor{*(processorPtr.get())};
    Connection           connection;
    EncoderBuilder       encoderBuilder{createEncoderBuilder()};
    EncoderMock*         encoderPtr;
    StreamingSessionTest session
    {
        processor.getProcessorProxy(),
        std::ref(connection),
        encoderBuilder
    };
};
