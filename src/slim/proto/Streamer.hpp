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
#include "slim/proto/Session.hpp"


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

				void onClose(ConnectionType& connection)
				{
					removeSession(connection);
				}

				void onData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					LOG(INFO) << "data callback receivedSize=" << receivedSize;

					if (!applyToSession(connection, [&](auto& session)
					{
						session.onData(buffer, receivedSize);
					}))
					{
						// TODO: refactor to a different class
						std::string helo{"HELO"};
						std::string s{(char*)buffer, helo.size()};
						if (!helo.compare(s))
						{
							LOG(INFO) << "HELO received";
							addSession(connection).onData(buffer, receivedSize);
						}
						else
						{
							LOG(INFO) << "Incorrect handshake message received";
							connection.stop();
						}
					}
				}

				void onOpen(ConnectionType& connection)
				{
					LOG(INFO) << "open callback";
				}

				void onStart(ConnectionType& connection)
				{
					LOG(INFO) << "start callback";
				}

				void onStop(ConnectionType& connection)
				{
					LOG(INFO) << "stop callback";
				}

			protected:
				auto& addSession(ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Adding new session (sessions=" << sessions.size() << ")...";

					auto result = std::find_if(sessions.begin(), sessions.end(), [&](auto& sessionPtr)
					{
						return &(sessionPtr->getConnection()) == &connection;
					});

					Session<ConnectionType>* s;
					if (result == sessions.end())
					{
						s = (sessions.emplace_back(std::make_unique<Session<ConnectionType>>(connection))).get();
						LOG(DEBUG) << LABELS{"slim"} << "New session was added (id=" << s << ", sessions=" << sessions.size() << ")";
					}
					else
					{
						s = (*result).get();
						LOG(INFO) << "Session already exists; probably client sent handshake message twice";
					}

					return *s;
				}

				bool applyToSession(ConnectionType& connection, std::function<void(Session<ConnectionType>&)> fun)
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

				void removeSession(ConnectionType& connection)
				{
					// removing session from the vector
					sessions.erase(
						std::remove_if(
							sessions.begin(),
							sessions.end(),
							[&](auto& sessionPtr) -> bool
							{
								return &(sessionPtr->getConnection()) == &connection;
							}
						),
						sessions.end()
					);
				}

			private:
				std::vector<std::unique_ptr<Session<ConnectionType>>> sessions;
		};
	}
}
