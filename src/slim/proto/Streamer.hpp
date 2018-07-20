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
#include <atomic>
#include <chrono>
#include <conwrap/ProcessorProxy.hpp>
#include <cstddef>  // std::size_t
#include <functional>
#include <memory>
#include <optional>
#include <sstream>  // std::stringstream
#include <string>
#include <thread>
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


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class Streamer : public Consumer
		{
			template<typename SessionType>
			using SessionsMap          = std::unordered_map<ConnectionType*, std::unique_ptr<SessionType>>;
			using TimePoint            = std::chrono::time_point<std::chrono::steady_clock>;
			using CommandSessionType   = CommandSession<ConnectionType>;
			using StreamingSessionType = StreamingSession<ConnectionType>;

			public:
				Streamer(unsigned int sp, EncoderBuilder eb, std::optional<unsigned int> g)
				: streamingPort{sp}
				, encoderBuilder{eb}
				, gain{g}
				, monitorThread{[&]
				{
					LOG(DEBUG) << LABELS{"proto"} << "Monitor thread was started (id=" << std::this_thread::get_id() << ")";

					for(unsigned int counter{0}; !monitorFinish; counter++, std::this_thread::sleep_for(std::chrono::milliseconds{200}))
			        {
						// TODO: make configurable
						if (counter > 24)
						{
							auto processorProxyPtr{getProcessorProxy()};
							if (processorProxyPtr)
							{
								processorProxyPtr->process([&]
								{
									// sending ping command to measure round-trip latency
									for (auto& entry : commandSessions)
									{
										entry.second->ping();
									}
								});
							}

							counter = 0;
						}
			        }

					LOG(DEBUG) << LABELS{"proto"} << "Monitor thread was stopped (id=" << std::this_thread::get_id() << ")";
				}}
				{
					LOG(DEBUG) << LABELS{"proto"} << "Streamer object was created (id=" << this << ")";
				}

				virtual ~Streamer()
				{
					// stopping monitor thread
					monitorFinish = true;
					if (monitorThread.joinable())
					{
						monitorThread.join();
					}

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
					auto chunkSamplingRate{chunk.getSamplingRate()};

					if (samplingRate != chunkSamplingRate)
					{
						if (samplingRate)
						{
							// propogating end-of-stream to all the clients
							stop();

							// changing state to not streaming
							streaming = false;
						}

						// assigning new sampling rate
						samplingRate = chunkSamplingRate;

						// if this is the beginning of a stream
						if (chunkSamplingRate)
						{
							start();
						}
					}
					else if (samplingRate)
					{
						if (!streaming)
						{
							// checking if all conditions for streaming were met
							streaming = isReadyToStream();
						}
						if (streaming)
						{
							// sending out Chunk to all the clients
							distributeChunk(chunk);

							// it will make Chunk to be consumed
							result = true;
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
							commandSession.value()->setStreamingSession(nullptr);
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

						// configuring an encoder builder
						encoderBuilder.setSamplingRate(samplingRate);
						encoderBuilder.setWriter(&connection);

						// creating streaming session object
						auto streamingSessionPtr{std::make_unique<StreamingSessionType>(std::ref<ConnectionType>(connection), std::move(encoderBuilder.build()), clientID.value())};

						// saving Streaming session reference in the relevant Command session
						auto commandSessionPtr{findSessionByID(commandSessions, clientID.value())};
						if (!commandSessionPtr.has_value())
						{
							throw slim::Exception("Could not correlate provided client ID with a valid SlimProto session");
						}
						commandSessionPtr.value()->setStreamingSession(streamingSessionPtr.get());

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

				void onSlimProtoData(ConnectionType& connection, unsigned char* buffer, std::size_t size)
				{
					try
					{
						if (!applyToSession(commandSessions, connection, [&](CommandSessionType& session)
						{
							session.onRequest(buffer, size);
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
					auto commandSessionPtr{std::make_unique<CommandSessionType>(std::ref<ConnectionType>(connection), ss.str(), streamingPort, encoderBuilder.getFormat(), gain)};

					// enable streaming for this session if required
					if (streaming)
					{
						commandSessionPtr->setSamplingRate(samplingRate);
						commandSessionPtr->start();
					}

					// saving command session in the map
					addSession(commandSessions, connection, std::move(commandSessionPtr));
				}

				virtual void start() override
				{
					for (auto& entry : commandSessions)
					{
						// TODO: this is a temporary solution
						entry.second->setSamplingRate(samplingRate);
						entry.second->start();
					}

					// saving when streaming was started - required for calculating defer time-out
					startedAt = std::chrono::steady_clock::now();
				}

				virtual void stop(bool gracefully = true) override
				{
					// stopping SlimProto session will send end-of-stream command which normally triggers close of HTTP connection
					// TODO: implement 'safety' logic to close HTTP connection in case client does not
					for (auto& entry : commandSessions)
					{
						entry.second->stop();
					}
				}

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

				inline void distributeChunk(Chunk& chunk)
				{
					// sending chunk to all HTTP sessions
					auto counter{commandSessions.size()};
					for (auto& entry : commandSessions)
					{
						auto streamingSessionPtr{entry.second->getStreamingSession()};
						if (streamingSessionPtr)
						{
							streamingSessionPtr->onChunk(chunk);
							counter--;
						}
					}

					// if there are command sessions without relevant HTTP session
					if (counter > 0)
					{
						LOG(WARNING) << LABELS{"proto"} << "A chunk was not delivered to all clients (clients=" << commandSessions.size() << ", skipped=" << counter << ", size=" << chunk.getSize() << ")";
					}
					else
					{
						LOG(DEBUG) << LABELS{"proto"} << "A chunk was delivered (clients=" << commandSessions.size() << ", size=" << chunk.getSize() << ")";
					}
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
					// TODO: deferring time-out should be configurable
					auto result{500 < std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt).count()};
					auto missingSessionsTotal{std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						auto streamingSessionPtr{entry.second->getStreamingSession()};

						return !(streamingSessionPtr && samplingRate == streamingSessionPtr->getSamplingRate());
					})};

					// if deferring time-out has expired
					if (result && missingSessionsTotal)
					{
						LOG(WARNING) << LABELS{"proto"} << "Could not defer chunk processing due to reached threashold";
					}
					else if (!result)
					{
						// if all HTTP sessions were established
						if (!missingSessionsTotal)
						{
							result = true;
						}
						else
						{
							LOG(DEBUG) << LABELS{"proto"} << "Deferring chunk transmition due to missing HTTP sessions";

							// TODO: implement cruise control; for now sleep is good enough
							// this sleep prevents from busy spinning until all HTTP sessions reconnect
							std::this_thread::sleep_for(std::chrono::milliseconds{20});
						}
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

			private:
				unsigned int                      streamingPort;
				EncoderBuilder                    encoderBuilder;
				std::optional<unsigned int>       gain;
				SessionsMap<CommandSessionType>   commandSessions;
				SessionsMap<StreamingSessionType> streamingSessions;
				bool                              streaming{false};
				unsigned int                      samplingRate{0};
				unsigned long                     nextID{0};
				std::atomic<bool>                 monitorFinish{false};
				std::thread                       monitorThread;
				TimePoint                         startedAt;
		};
	}
}
