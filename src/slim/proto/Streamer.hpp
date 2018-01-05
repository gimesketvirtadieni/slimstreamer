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

#include <functional>
#include <memory>
#include <vector>

#include "slim/log/log.hpp"
#include "slim/proto/CommandSession.hpp"
#include "slim/proto/StreamingSession.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class Streamer
		{
			public:
				// using Rule Of Zero
				Streamer() = default;
			   ~Streamer() = default;
				Streamer(const Streamer&) = delete;             // non-copyable
				Streamer& operator=(const Streamer&) = delete;  // non-assignable
				Streamer(Streamer&& rhs) = delete;              // non-movable
				Streamer& operator=(Streamer&& rhs) = delete;   // non-movable-assinable

				inline void consume(Chunk& chunk)
				{
					for (auto& sessionPtr : streamingSessions)
					{
						sessionPtr->consume(chunk);
					}
				}

				void onHTTPClose(ConnectionType& connection)
				{
					LOG(INFO) << "HTTP close callback";

					removeSession(streamingSessions, connection);
				}

				void onHTTPData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					LOG(INFO) << "HTTP data callback receivedSize=" << receivedSize;

					if (!applyToSession(streamingSessions, connection, [&](StreamingSession<ConnectionType>& session)
					{
						session.onData(buffer, receivedSize);
					}))
					{
						// TODO: work in progress
						LOG(INFO) << "HTTP request received";

						// TODO: refactor to a different class
						std::string get{"GET"};
						std::string s{(char*)buffer, get.size()};
						if (!get.compare(s))
						{
							LOG(INFO) << "HTTP GET request received";

							addSession(streamingSessions, connection).onData(buffer, receivedSize);
						}
					}
				}

				void onHTTPOpen(ConnectionType& connection)
				{
					LOG(INFO) << "HTTP open callback";
				}

				void onHTTPStart(ConnectionType& connection)
				{
					LOG(INFO) << "HTTP start callback";
				}

				void onHTTPStop(ConnectionType& connection)
				{
					LOG(INFO) << "HTTP stop callback";
				}

				void onSlimProtoClose(ConnectionType& connection)
				{
					LOG(INFO) << "SlimProto close callback";

					removeSession(commandSessions, connection);
				}

				void onSlimProtoData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					LOG(INFO) << "SlimProto data callback receivedSize=" << receivedSize;

					if (!applyToSession(commandSessions, connection, [&](CommandSession<ConnectionType>& session)
					{
						session.onData(buffer, receivedSize);
					}))
					{
						// TODO: refactor to a different class
						std::string helo{"HELO"};
						std::string s{(char*)buffer, helo.size()};
						if (!helo.compare(s))
						{
							LOG(INFO) << "HELO command received";

							addSession(commandSessions, connection).onData(buffer, receivedSize);
						}
						else
						{
							LOG(INFO) << "Incorrect handshake message received";

							connection.stop();
						}
					}
				}

				void onSlimProtoOpen(ConnectionType& connection)
				{
					LOG(INFO) << "SlimProto open callback";
				}

				void onSlimProtoStart(ConnectionType& connection)
				{
					LOG(INFO) << "SlimProto start callback";
				}

				void onSlimProtoStop(ConnectionType& connection)
				{
					LOG(INFO) << "SlimProto stop callback";
				}

			protected:
				template<typename SessionType>
				auto& addSession(std::vector<std::unique_ptr<SessionType>>& sessions, ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Adding new session (sessions=" << sessions.size() << ")...";

					auto result = std::find_if(sessions.begin(), sessions.end(), [&](auto& sessionPtr)
					{
						return &(sessionPtr->getConnection()) == &connection;
					});

					SessionType* s{nullptr};
					if (result == sessions.end())
					{
						s = (sessions.emplace_back(std::make_unique<SessionType>(connection))).get();
						LOG(DEBUG) << LABELS{"slim"} << "New session was added (id=" << s << ", sessions=" << sessions.size() << ")";
					}
					else
					{
						s = (*result).get();
						LOG(INFO) << "Session already exists";
					}

					return *s;
				}

				template<typename SessionType, typename FunctionType>
				bool applyToSession(std::vector<std::unique_ptr<SessionType>>& sessions, ConnectionType& connection, FunctionType fun)
				{
					return sessions.end() != std::find_if(sessions.begin(), sessions.end(), [&](auto& sessionPtr)
					{
						auto found{false};

						if (&(sessionPtr->getConnection()) == &connection)
						{
							found = true;
							fun(*sessionPtr);
						}

						return found;
					});
				}

				template<typename SessionType>
				void removeSession(std::vector<std::unique_ptr<SessionType>>& sessions, ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Removing session (sessions=" << sessions.size() << ")...";

					// removing session from the vector
					SessionType* s{nullptr};
					sessions.erase(std::remove_if(sessions.begin(), sessions.end(), [&](auto& sessionPtr) -> bool
					{
						auto found{false};

						if (&(sessionPtr->getConnection()) == &connection)
						{
							s     = sessionPtr.get();
							found = true;
						}

						return found;
					}), sessions.end());

					LOG(DEBUG) << LABELS{"slim"} << "Session was removed (id=" << s << ", sessions=" << sessions.size() << ")";
				}

			private:
				std::vector<std::unique_ptr<CommandSession<ConnectionType>>>   commandSessions;
				std::vector<std::unique_ptr<StreamingSession<ConnectionType>>> streamingSessions;
		};
	}
}
