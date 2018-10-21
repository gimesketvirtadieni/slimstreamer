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

#pragma once

#include <algorithm>
#include <chrono>
#include <conwrap2/ProcessorProxy.hpp>
#include <cstddef>  // std::size_t
#include <cstdint>  // std::u..._t types
#include <functional>
#include <memory>
#include <sstream>  // std::stringstream
#include <string>
#include <type_safe/optional.hpp>
#include <type_safe/optional_ref.hpp>
#include <unordered_map>
#include <vector>

#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/proto/Command.hpp"
#include "slim/proto/CommandSession.hpp"
#include "slim/proto/StreamingSession.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace proto
	{
		namespace ts = type_safe;

		template<typename ConnectionType>
		class Streamer : public Consumer
		{
			template<typename SessionType>
			using SessionsMap          = std::unordered_map<ConnectionType*, std::unique_ptr<SessionType>>;
			using CommandSessionType   = CommandSession<ConnectionType>;
			using StreamingSessionType = StreamingSession<ConnectionType>;

			public:
				Streamer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, unsigned int sp, EncoderBuilder eb, std::optional<unsigned int> ga)
				: Consumer{pp, 0}
				, streamingPort{sp}
				, encoderBuilder{eb}
				, gain{ga}
				{
					LOG(DEBUG) << LABELS{"proto"} << "Streamer object was created (id=" << this << ")";
				}

				virtual ~Streamer()
				{
					LOG(DEBUG) << LABELS{"proto"} << "Streamer object was deleted (id=" << this << ")";
				}

				// TODO: non-movable is required due to usage in server callbacks; consider refactoring
				Streamer(const Streamer&) = delete;             // non-copyable
				Streamer& operator=(const Streamer&) = delete;  // non-assignable
				Streamer(Streamer&& rhs) = delete;              // non-movable
				Streamer& operator=(Streamer&& rhs) = delete;   // non-movable-assinable

				virtual bool consumeChunk(Chunk& chunk) override
				{
					auto result{false};
					auto samplingRate{getSamplingRate()};
					auto chunkSamplingRate{chunk.getSamplingRate()};

					// if PCM stream changes
					if (samplingRate != chunkSamplingRate)
					{
						// if streaming is ongoing
						if (samplingRate)
						{
							// propogating end-of-stream to all the clients
							stopStreaming();

							// changing state to not streaming
							streaming = false;
						}

						// assigning new sampling rate
						setSamplingRate(chunkSamplingRate);

						// if this is the beginning of a stream
						if (chunkSamplingRate)
						{
							startStreaming();
						}
					}
					// if streaming is ongoing without changes
					else if (samplingRate)
					{
						// if streaming is being initialized
						if (!streaming)
						{
							// checking if all conditions for streaming were met
							streaming = isReadyToStream();
						}

						// if streaming is all set
						if (streaming)
						{
							// sending out Chunk to all the clients
							distributeChunk(chunk);

							// it will make Chunk to be consumed
							result = true;
							streamingDuration += chunk.getDurationMicroseconds();
						}
					}
					else
					{
						// consuming Chunk in case when samplingRate == chunkSamplingRate == 0
						result = true;
					}

					return result;
				}

				void onHTTPClose(ConnectionType& connection)
				{
					LOG(INFO) << LABELS{"proto"} << "HTTP close callback (connection=" << &connection << ")";

					auto streamingSessionPtr{removeSession(streamingSessions, connection)};
					if (streamingSessionPtr)
					{
						// if there is a relevant SlimProto connection found
						auto clientID{streamingSessionPtr->getClientID()};
						auto commandSession{findSessionByID(commandSessions, clientID)};
						if (commandSession.has_value())
						{
							// resetting HTTP session in its relevant SlimProto session
							commandSession.value()->setStreamingSession(ts::optional_ref<StreamingSessionType>{ts::nullopt});
						}
						else
						{
							LOG(WARNING) << LABELS{"proto"} << "Could not find SlimProto session by client ID (clientID=" << clientID << ")";
						}
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Could not find HTTP session object";
					}
				}

				void onHTTPData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					if (!applyToSession(streamingSessions, connection, [&](StreamingSessionType& session)
					{
						// processing request by a proper Streaming session mapped to this connection
						session.onRequest(buffer, receivedSize);
					}))
					{
						// parsing client ID
						auto clientID = StreamingSessionType::parseClientID(std::string{(char*)buffer, receivedSize});
						if (!clientID.has_value())
						{
							throw slim::Exception("Missing client ID in streaming session request");
						}

						LOG(INFO) << LABELS{"proto"} << "Client ID was parsed (clientID=" << clientID.value() << ")";

						// setting encoder sampling rate
						encoderBuilder.setSamplingRate(getSamplingRate());

						// creating streaming session object
						auto streamingSessionPtr{std::make_unique<StreamingSessionType>(std::ref<ConnectionType>(connection), clientID.value(), encoderBuilder)};

						// saving Streaming session reference in the relevant Command session
						auto commandSession{findSessionByID(commandSessions, clientID.value())};
						if (!commandSession.has_value())
						{
							connection.stop();
							throw slim::Exception("Could not correlate provided client ID with a valid SlimProto session");
						}
						commandSession.value()->setStreamingSession(ts::optional_ref<StreamingSessionType>{*streamingSessionPtr});

						// processing request by a proper Streaming session mapped to this connection
						streamingSessionPtr->onRequest(buffer, receivedSize);

						// saving Streaming session as a part of this Streamer
						addSession(streamingSessions, connection, std::move(streamingSessionPtr));
					}
				}

				void onHTTPOpen(ConnectionType& connection)
				{
					LOG(INFO) << LABELS{"proto"} << "New HTTP session request received (connection=" << &connection << ")";
				}

				void onSlimProtoClose(ConnectionType& connection)
				{
					removeSession(commandSessions, connection);
				}

				void onSlimProtoData(ConnectionType& connection, unsigned char* buffer, std::size_t size, util::Timestamp timestamp)
				{
					try
					{
						if (!applyToSession(commandSessions, connection, [&](CommandSessionType& session)
						{
							session.onRequest(buffer, size, timestamp);
						}))
						{
							throw slim::Exception("Could not find SlimProto session object");
						}
					}
					catch (const slim::Exception& error)
					{
						LOG(ERROR) << LABELS{"proto"} << "Error while processing SlimProto command: " << error.what();
						connection.stop();
					}
				}

				void onSlimProtoOpen(ConnectionType& connection)
				{
					// using regular counter for session ID's instead of MAC's; it allows running multiple players on one host
					std::stringstream ss;
					ss << (++nextID);

					// creating command session object
					auto commandSessionPtr{std::make_unique<CommandSessionType>(getProcessorProxy(), std::ref<ConnectionType>(connection), ss.str(), streamingPort, encoderBuilder.getFormat(), gain)};

					// enable streaming for this session if required
					if (streaming)
					{
						commandSessionPtr->setSamplingRate(getSamplingRate());
						commandSessionPtr->start();
					}

					// saving command session in the map
					addSession(commandSessions, connection, std::move(commandSessionPtr));
				}

				virtual void setSamplingRate(unsigned int s) override
				{
					Consumer::setSamplingRate(s);

					for (auto& entry : commandSessions)
					{
						// setting new sampling rate to all the clients
						entry.second->setSamplingRate(s);

						// resetting links between SlimProto and HTTP sessions to keep track on HTTP sessions reconnects
						entry.second->setStreamingSession(nullptr);
					}
				}

				virtual void start() override {}
				virtual void stop(bool gracefully = true) override {}

			protected:
				template<typename SessionType>
				inline auto& addSession(SessionsMap<SessionType>& sessions, ConnectionType& connection, std::unique_ptr<SessionType> sessionPtr)
				{
					auto         found{sessions.find(&connection)};
					SessionType* s{nullptr};

					if (found == sessions.end())
					{
						s = sessionPtr.get();

						// saving session in a map; using pointer to a relevant connection as an ID
						sessions[&connection] = std::move(sessionPtr);
						LOG(DEBUG) << LABELS{"proto"} << "New session was added (id=" << s << ", sessions=" << sessions.size() << ")";
					}
					else
					{
						s = (*found).second.get();
						LOG(WARNING) << LABELS{"proto"} << "Session already exists";
					}

					return *s;
				}

				template<typename SessionType, typename FunctionType>
				inline bool applyToSession(SessionsMap<SessionType>& sessions, ConnectionType& connection, FunctionType fun)
				{
					auto found{sessions.find(&connection)};

					if (found != sessions.end())
					{
						fun(*(*found).second);
					}

					return (found != sessions.end());
				}

				inline auto calculatePlaybackStartTime()
				{
					unsigned long long maxLatency{0};

					for (auto& entry : commandSessions)
					{
						entry.second->getLatency().map([&](auto& latency)
						{
							if (maxLatency < latency)
							{
								maxLatency = latency;
							}
						});
					}

					return util::Timestamp::now() + std::chrono::milliseconds{maxLatency};
				}

				inline void distributeChunk(Chunk& chunk)
				{
					// sending chunk to all SlimProto sessions
					for (auto& entry : commandSessions)
					{
						entry.second->onChunk(chunk);
					}

					LOG(DEBUG) << LABELS{"proto"} << "A chunk was distributed to the clients (total clients=" << commandSessions.size() << ", frames=" << chunk.getFrames() << ")";
				}

				template<typename SessionType>
				auto findSessionByID(SessionsMap<SessionType>& sessions,  std::string clientID)
				{
					auto result{std::optional<SessionType*>{std::nullopt}};
					auto found{std::find_if(sessions.begin(), sessions.end(), [&](auto& entry) -> bool
					{
						auto found{false};

						if (entry.second->getClientID() == clientID)
						{
							found = true;
						}

						return found;
					})};

					if (found != sessions.end())
					{
						result = (*found).second.get();
					}

					return result;
				}

				inline auto isReadyToStream()
				{
					auto result{false};
					// TODO: deferring time-out should be configurable
					auto waitThresholdReached{500000 < (util::Timestamp::now().get(util::microseconds) - streamingStartedAt.get(util::microseconds))};
					auto missingSessionsTotal{std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						return !entry.second->getStreamingSession();
					})};

					// if deferring time-out has not expired
					if (!waitThresholdReached)
					{
						// if all HTTP sessions were established
						if (!missingSessionsTotal)
						{
							result = true;
						}
						else
						{
							// TODO: implement cruise control; for now sleep is good enough
							// this sleep prevents from busy spinning until all HTTP sessions reconnect
							// potentially this sleep may interfere with latency measuments so it should be as low as possible
							std::this_thread::sleep_for(std::chrono::microseconds{500});
						}
					}
					else
					{
						if (missingSessionsTotal)
						{
							LOG(WARNING) << LABELS{"proto"} << "Could not defer chunk processing due to reached threshold";
						}
						result = true;
					}

					return result;
				}

				template<typename SessionType>
				inline auto removeSession(SessionsMap<SessionType>& sessions, ConnectionType& connection)
				{
					std::unique_ptr<SessionType> sessionPtr{};
					auto                         found{sessions.find(&connection)};

					if (found != sessions.end())
					{
						sessionPtr = std::move((*found).second);
						sessions.erase(found);
						LOG(DEBUG) << LABELS{"proto"} << "Session was removed (id=" << sessionPtr.get() << ", sessions=" << sessions.size() << ")";
					}

					return std::move(sessionPtr);
				}

				inline void startStreaming()
				{
					// capturing start stream time point (required for calculations like defer time-out, etc.)
					streamingStartedAt = util::Timestamp::now();
					streamingDuration  = 0;

					// TODO: work in progress
					// calculating the timepoint when clients start playing audio
					calculatePlaybackStartTime();

					for (auto& entry : commandSessions)
					{
						entry.second->start();
					}

					LOG(DEBUG) << LABELS{"proto"} << "Started streaming (rate=" << getSamplingRate() << ")";
				}

				inline void stopStreaming()
				{
					// capturing stop streaming time point
					streamingStoppedAt = util::Timestamp::now();

					// it is enough to send stop SlimProto command here
					for (auto& entry : commandSessions)
					{
						// stopping SlimProto session will send end-of-stream command which normally triggers close of HTTP connection
						entry.second->stop();
					}

					LOG(DEBUG) << LABELS{"proto"} << "Stopped streaming (duration=" << streamingDuration  << " microsec)";
				}

			private:
				unsigned int                      streamingPort;
				EncoderBuilder                    encoderBuilder;
				std::optional<unsigned int>       gain;
				unsigned long long                nextID{0};
				SessionsMap<CommandSessionType>   commandSessions;
				SessionsMap<StreamingSessionType> streamingSessions;
				bool                              streaming{false};
				unsigned long long                streamingDuration{0};
				util::Timestamp                   streamingStartedAt;
				util::Timestamp                   streamingStoppedAt;
		};
	}
}
