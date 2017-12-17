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

#include "slim/log/log.hpp"
#include "slim/Server.hpp"


namespace slim
{
	void Server::addSession()
	{
		auto startCallback = [](auto&)
		{
			LOG(INFO) << "start callback";
		};

		auto openCallback = [&](auto& session)
		{
			LOG(INFO) << "open callback";

			// registering a new session if capacity allows so new requests can be accepted
			if (sessions.size() < maxSessions)
			{
				addSession();
			} else {
				LOG(DEBUG) << LABELS{"slim"} << "Limit of active sessions was reached (id=" << this << ", sessions=" << sessions.size() << " max=" << maxSessions << ")";
				stopAcceptor();
			}
		};

		auto closeCallback = [&](auto& session)
		{
			LOG(INFO) << "close callback";
		};

		auto stopCallback = [&](auto& session)
		{
			LOG(INFO) << "stop callback";

			// session cannot be removed at this moment as this method is called withing this session
			processorProxyPtr->process([&]
			{
				removeSession(session);

				// starting acceptor if required and adding new session to accept connection
				if (started && !acceptorPtr) {
					startAcceptor();
					addSession();
				}
			});
		};

		LOG(DEBUG) << LABELS{"slim"} << "Adding new session (sessions=" << sessions.size() << ")...";

		// creating new session
		auto sessionPtr{std::make_unique<Session>(processorProxyPtr, std::move(startCallback), std::move(openCallback), std::move(closeCallback), std::move(stopCallback))};

		// start accepting connection
		sessionPtr->start(*acceptorPtr);

		// adding session to the sessions vector
		sessions.push_back(std::move(sessionPtr));

		LOG(DEBUG) << LABELS{"slim"} << "New session was added (id=" << this << ", sessions=" << sessions.size() << ")";
	}


	void Server::removeSession(Session& session)
	{
		auto sessionPtr{&session};

		LOG(DEBUG) << LABELS{"slim"} << "Removing session (id=" << sessionPtr << ", sessions=" << sessions.size() << ")...";

		// removing session from the vector
		sessions.erase(
			std::remove_if(
				sessions.begin(),
				sessions.end(),
				[&](auto& sessionPtr) -> bool
				{
					return sessionPtr.get() == &session;
				}
			),
			sessions.end()
		);

		LOG(DEBUG) << LABELS{"slim"} << "Session was removed (id=" << sessionPtr << ", sessions=" << sessions.size() << ")";
	}


	void Server::setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerBase>* p)
	{
		processorProxyPtr = p;
	}


	void Server::start()
	{
		LOG(INFO) << LABELS{"slim"} << "Starting new server (id=" << this << ", port=" << port << ", max sessions=" << maxSessions << ")...";

		// start accepting new sessions
		startAcceptor();
		addSession();

		started = true;
		LOG(INFO) << LABELS{"slim"} << "Server was started (id=" << this << ")";
	}


	void Server::startAcceptor()
	{
		// creating an acceptor if required
		if (!acceptorPtr)
		{
			LOG(DEBUG) << LABELS{"slim"} << "Starting acceptor...";

			acceptorPtr = std::make_unique<asio::ip::tcp::acceptor>(
				*processorProxyPtr->getDispatcher(),
				asio::ip::tcp::endpoint(
					asio::ip::tcp::v4(),
					port
				)
			);

			LOG(DEBUG) << LABELS{"slim"} << "Acceptor was started (id=" << acceptorPtr.get() << ")";
		}
	}


	void Server::stop()
	{
		LOG(INFO) << LABELS{"slim"} << "Stopping server...";
		started = false;

		// stop accepting any new connections
		stopAcceptor();

		// closing active sessions; sessions will be removed by onStop callback
		for (auto& sessionPtr : sessions)
		{
			sessionPtr->stop();
		}

		LOG(INFO) << LABELS{"slim"} << "Server was stopped (id=" << this << ")";
	}


	void Server::stopAcceptor() {

		// disposing acceptor to prevent from new incomming requests
		if (acceptorPtr) {
			LOG(DEBUG) << LABELS{"slim"} << "Stopping acceptor (id=" << acceptorPtr.get() << ")...";

			acceptorPtr->cancel();
			acceptorPtr->close();

			// acceptor is not captured by handlers so it is safe to delete it
			acceptorPtr.reset();

			LOG(DEBUG) << LABELS{"slim"} << "Acceptor was stopped (id=" << acceptorPtr.get() << ")";
		}
	}
}
