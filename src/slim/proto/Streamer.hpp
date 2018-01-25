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
#include <conwrap/ProcessorProxy.hpp>
#include <cstddef>  // std::size_t
#include <memory>
#include <optional>
#include <sstream>  // std::stringstream
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/proto/CommandHELO.hpp"
#include "slim/proto/CommandSession.hpp"
#include "slim/proto/StreamingSession.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class Streamer
		{
			template<typename SessionType>
			using SessionsMap = std::unordered_map<ConnectionType*, std::unique_ptr<SessionType>>;
			using TimePoint   = std::chrono::time_point<std::chrono::steady_clock>;

			public:
				Streamer()
				: timerThread{[&]
				{
					LOG(DEBUG) << "Timer thread started";

					for(unsigned int counter{0}; timerRunning; counter++, std::this_thread::sleep_for(std::chrono::milliseconds{200}))
			        {
						if (counter > 24)
						{
							// TODO: use std::optional
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

					LOG(DEBUG) << "Timer thread stopped";
				}} {}

			   ~Streamer()
				{
					timerRunning = false;
					timerThread.join();
				}

				Streamer(const Streamer&) = delete;             // non-copyable
				Streamer& operator=(const Streamer&) = delete;  // non-assignable
				Streamer(Streamer&& rhs) = delete;              // non-movable
				Streamer& operator=(Streamer&& rhs) = delete;   // non-movable-assinable

				inline bool onChunk(Chunk& chunk, unsigned int sr)
				{
					if (sr && samplingRate != sr)
					{
						// resetting current sampling rate to zero so futher routine can handle it
						samplingRate = 0;

						// stopping all streaming sessions which will make them reconnect using a new sampling rate
						for (auto& entry : streamingSessions)
						{
							entry.first->stop();
						}

						// creating a vector with all command sessions that require a relevant HTTP session
						for (auto& entry : commandSessions)
						{
							unmatchedSessions.push_back(entry.second.get());
						}
					}

					auto done{true};
					if (sr && !samplingRate)
					{
						// deferring chunk transmition and setting a new sampling rate
						done         = false;
						samplingRate = sr;

						// sending command to start streaming
						for (auto& entry : commandSessions)
						{
					        entry.second->stream(samplingRate);
						}
					}

					if (sr && samplingRate == sr && done)
					{
						auto s{unmatchedSessions.size()};

						// if there are command sessions without relevant HTTP session
						if (s > 0)
						{
							// if there is time for deferring chunk processing
							if (!hasToFinish())
							{
								LOG(DEBUG) << "Deferring chunk transmition due to missing HTTP sessions";
								done = false;

								// TODO: implement cruise control; for now sleep is good enough
								// this sleep prevents from busy spinning until all HTTP sessions reconnect
								std::this_thread::sleep_for(std::chrono::milliseconds{20});
							}
							else
							{
								LOG(DEBUG) << "Could not defer chunk processing due to reached threashold";
							}
						}

						// if there is no need to defer chunk processing
						if (done)
						{
							// resetting period during which chunk processing can be deferred
							deferStartedAt.reset();

							// sending chunk to all HTTP sessions
							for (auto& entry : streamingSessions)
							{
								entry.second->onChunk(chunk, samplingRate);
							}

							// if there are command sessions without relevant HTTP session
							if (s > 0)
							{
								LOG(WARNING) << "Current chunk transmition was skipped for " << s << " client(s)";
							}
						}
					}

					return done;
				}

				void onHTTPClose(ConnectionType& connection)
				{
					LOG(INFO) << "HTTP close callback";

					auto streamingSessionPtr{removeSession(streamingSessions, connection)};
					auto clientID{streamingSessionPtr->getClientID()};
					auto commandSession{findCommandSession(clientID)};

					// if there is a relevant SlimProto connection found
					if (commandSession.has_value())
					{
						auto found{std::find_if(unmatchedSessions.begin(), unmatchedSessions.end(), [&](auto& entry) -> bool
						{
							return entry == commandSession.value();
						})};

						// adding command session pointer if it is missing in vector
						if (found == unmatchedSessions.end())
						{
							unmatchedSessions.push_back({commandSession.value()});
						}
					}
				}

				void onHTTPData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					LOG(INFO) << "HTTP data callback receivedSize=" << receivedSize;

					if (!applyToSession(streamingSessions, connection, [&](StreamingSession<ConnectionType>& session)
					{
						session.onRequest(buffer, receivedSize);
					}))
					{
						LOG(INFO) << "HTTP request received";

						try
						{
							// TODO: make more strick validation
							std::string get{"GET"};
							std::string s{(char*)buffer, get.size()};
							if (get.compare(s))
							{
								throw slim::Exception("Wrong method provided");
							}

							auto clientID{StreamingSession<ConnectionType>::parseClientID({(char*)buffer, receivedSize})};
							if (!clientID.has_value())
							{
								throw slim::Exception("Missing client ID in HTTP request");
							}

							LOG(INFO) << "Client ID was parsed from HTTP request (clientID=" << clientID.value() << ")";

							// if there a SlimProto connection found that originated this HTTP request
							auto commandSessionPtr{findCommandSession(clientID.value()).value_or(nullptr)};
							if (!commandSessionPtr)
							{
								throw slim::Exception("Could not correlate provided client ID with a valid SlimProto session");
							}

							// creating streaming session object
							auto streamingSessionPtr{std::make_unique<StreamingSession<ConnectionType>>(connection, clientID.value(), 2, samplingRate, 32)};
							streamingSessionPtr->onRequest(buffer, receivedSize);
							addSession(streamingSessions, connection, std::move(streamingSessionPtr));

							// removing command session from unmatched sessions vector
							unmatchedSessions.erase(std::remove_if(unmatchedSessions.begin(), unmatchedSessions.end(), [&](auto& entry) -> bool
							{
								return entry == commandSessionPtr;
							}), unmatchedSessions.end());
						}
						catch (const slim::Exception& error)
						{
							LOG(ERROR) << "Incorrect HTTP request received: " << error.what();
							connection.stop();
						}
					}
				}

				void onHTTPOpen(ConnectionType& connection)
				{
					LOG(INFO) << "HTTP open callback";
				}

				void onSlimProtoClose(ConnectionType& connection)
				{
					LOG(INFO) << "SlimProto close callback";

					auto commandSessionPtr{removeSession(commandSessions, connection)};

					// removing command session from unmatched sessions vector
					unmatchedSessions.erase(std::remove_if(unmatchedSessions.begin(), unmatchedSessions.end(), [&](auto& entry) -> bool
					{
						return entry == commandSessionPtr.get();
					}), unmatchedSessions.end());
				}

				void onSlimProtoData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					if (!applyToSession(commandSessions, connection, [&](CommandSession<ConnectionType>& session)
					{
						session.onRequest(buffer, receivedSize);
					}))
					{
						// TODO: move to CommandSession due to command buffering per session
						// deserializing HELO command
						try
						{
							CommandHELO commandHELO{buffer, receivedSize};

							LOG(INFO) << "HELO command received";

							// not using MAC as a client ID to allow possibility running multiple players on one host
							std::stringstream ss;
					        ss << (nextID++);

					        // creating command session object
							auto sessionPtr{std::make_unique<CommandSession<ConnectionType>>(connection, ss.str(), commandHELO)};

							// saving command session in the map
							addSession(commandSessions, connection, std::move(sessionPtr));
						}
						catch (const slim::Exception& error)
						{
							LOG(ERROR) << "Incorrect HELO command received: " << error.what();
							connection.stop();
						}
					}
				}

				void onSlimProtoOpen(ConnectionType& connection)
				{
					LOG(INFO) << "SlimProto open callback";
				}

				void setProcessorProxy(conwrap::ProcessorProxy<ContainerBase>* p)
				{
					processorProxyPtr = p;
				}

			protected:
				template<typename SessionType>
				inline auto& addSession(SessionsMap<SessionType>& sessions, ConnectionType& connection, std::unique_ptr<SessionType> sessionPtr)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Adding new session (sessions=" << sessions.size() << ")...";

					auto         found{sessions.find(&connection)};
					SessionType* s{nullptr};

					if (found == sessions.end())
					{
						s = sessionPtr.get();

						// saving session in a map; using pointer to a relevant connection as an ID
						sessions[&connection] = std::move(sessionPtr);
						LOG(DEBUG) << LABELS{"slim"} << "New session was added (id=" << s << ", sessions=" << sessions.size() << ")";
					}
					else
					{
						s = (*found).second.get();
						LOG(INFO) << "Session already exists";
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

				auto findCommandSession(std::string clientID)
				{
					auto result{std::optional<CommandSession<ConnectionType>*>{std::nullopt}};
					auto found{std::find_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry) -> bool
					{
						auto found{false};

						if (entry.second->getClientID() == clientID)
						{
							found = true;
						}

						return found;
					})};

					if (found != commandSessions.end())
					{
						result.emplace((*found).second.get());
					}

					return result;
				}

				inline bool hasToFinish()
				{
					auto finish{false};

					if (deferStartedAt.has_value())
					{
						auto diff{std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - deferStartedAt.value()).count()};
						finish = (diff > 100);
					}
					else
					{
						deferStartedAt = std::chrono::steady_clock::now();
					}

					return finish;
				}

				template<typename SessionType>
				inline auto removeSession(SessionsMap<SessionType>& sessions, ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Removing session (sessions=" << sessions.size() << ")...";

					auto sessionPtr{std::unique_ptr<SessionType>{}};
					auto found{sessions.find(&connection)};

					if (found != sessions.end())
					{
						sessionPtr = std::move((*found).second);
						sessions.erase(found);
						LOG(DEBUG) << LABELS{"slim"} << "Session was removed (id=" << sessionPtr.get() << ", sessions=" << sessions.size() << ")";
					}

					return std::move(sessionPtr);
				}

			private:
				SessionsMap<CommandSession<ConnectionType>>   commandSessions;
				SessionsMap<StreamingSession<ConnectionType>> streamingSessions;
				std::vector<CommandSession<ConnectionType>*>  unmatchedSessions;
				unsigned int                                  samplingRate{0};
				unsigned long                                 nextID{1};
				conwrap::ProcessorProxy<ContainerBase>*       processorProxyPtr{nullptr};
				volatile bool                                 timerRunning{true};
				std::thread                                   timerThread;
				std::optional<TimePoint>                      deferStartedAt{std::nullopt};
		};
	}
}
