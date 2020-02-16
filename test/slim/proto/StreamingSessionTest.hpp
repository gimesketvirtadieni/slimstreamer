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

#include <atomic>
#include <conwrap2/Processor.hpp>
#include <conwrap2/ProcessorProxy.hpp>
#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <queue>
#include <memory>
#include <ofats/invocable.h>

#include "slim/Chunk.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/proto/StreamingSession.hpp"


struct StreamingSessionFixture : public ::testing::TestWithParam<std::size_t>
{
    struct ConnectionMock
    {
        struct WriteAsyncRequest
        {
            std::string                  data;
            ofats::any_invocable<void()> callback;
        };

        void flush()
        {
            while (!requests.empty())
            {
                writtenData << requests.front().data;
                requests.front().callback();
                requests.pop();
            }
        }

        void stop()
        {
            stopCalledTimes++;
            stopCalledSequence = ++invocationsCounter;
        }

        template<class CallbackType>
        void writeAsync(const void* data, const std::size_t size, CallbackType callback = [](auto, auto) {})
        {
            requests.push(WriteAsyncRequest{"", [=, callback = std::move(callback)]
            {
                callback(std::error_code{}, size);
            }});
        }

        void write(std::string str)
        {
            writtenData << str;
        }

        std::size_t write(const void* data, const std::size_t size)
        {
            writtenData << std::string{(const char*) data, size};
            return size;
        }

        std::atomic<unsigned int>     stopCalledTimes{0};
        std::atomic<unsigned int>     stopCalledSequence;
        std::queue<WriteAsyncRequest> requests;
        std::stringstream             writtenData;
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

        virtual void encode(unsigned char* data, const std::size_t size) override
        {
            encodeCalledTimes++;
            encodeCalledSequence = ++invocationsCounter;
        }

        virtual bool isRunning()
        {
            return true;
        }

        virtual void start()
        {
            startCalledTimes++;
            startCalledSequence = ++invocationsCounter;
        }

        virtual void stop(ofats::any_invocable<void()> callback)
        {
            stopCalledTimes++;
            stopCalledSequence = ++invocationsCounter;
            callback();
        }

        std::atomic<unsigned int> encodeCalledTimes{0};
        std::atomic<unsigned int> encodeCalledSequence;
        std::atomic<unsigned int> startCalledTimes{0};
        std::atomic<unsigned int> startCalledSequence;
        std::atomic<unsigned int> stopCalledTimes{0};
        std::atomic<unsigned int> stopCalledSequence;
    };

    using Chunk                = slim::Chunk;
    using Processor            = conwrap2::Processor<std::unique_ptr<slim::ContainerBase>>;
    using Connection           = ConnectionMock;
    using EncoderBuilder       = slim::EncoderBuilder;
    using Encoder              = EncoderMock;
    using StreamingSessionTest = slim::proto::StreamingSession<ConnectionMock>;

    StreamingSessionFixture()
    {
        invocationsCounter = 0;
    }

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
            encoderPtr = new Encoder{ch, bs, bv, sr, ex, mm, ec};
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

    auto processAndWait(ofats::any_invocable<void()>&& handler)
    {
        auto p = std::promise<bool>{};
        auto f = p.get_future();
        processor.process([handler = std::move(handler), p = std::move(p)]() mutable
        {
            handler();
            p.set_value(true);
        });

        return f.wait_for(std::chrono::milliseconds(10));
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
    Encoder*             encoderPtr;
    StreamingSessionTest session
    {
        processor.getProcessorProxy(),
        std::ref(connection),
        encoderBuilder
    };

    // this counter is used to track invocation sequence
    static std::atomic<unsigned int> invocationsCounter;
};
