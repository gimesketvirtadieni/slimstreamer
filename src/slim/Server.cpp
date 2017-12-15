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
	std::unique_ptr<Session> Server::createSession()
	{
		// creating new session
		return std::make_unique<Session>(processorProxyPtr, [](auto&)
		{
			LOG(INFO) << "start callback";
		}, [](auto&)
		{
			LOG(INFO) << "open callback";
		}, [](Session& session)
		{
			LOG(INFO) << "close callback";
		}, [&](auto& session)
		{
			LOG(INFO) << "stop callback";

			// session cannot be deleted at this moment as this method is called withing this session
			processorProxyPtr->process([&]
			{
				deleteSession(session);
			});
		});
	}


	void Server::deleteSession(Session& session)
	{
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

		// creating new session required to accept connections
		auto sessionPtr{createSession()};

		// start accepting on this newly created session
		sessionPtr->start(*acceptorPtr);

		// adding session into a vector so it can be used later on
		sessions.push_back(std::move(sessionPtr));

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

		// stop accepting any new connections
		stopAcceptor();

		// closing active sessions; sessions will be deleted by onStop callback
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
