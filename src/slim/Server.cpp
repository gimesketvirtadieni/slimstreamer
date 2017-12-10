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
/*
	Session Server::createSession()
	{
		// creating session object per connection
		auto processorSessionPtr = std::make_unique<conwrap::ProcessorQueue<Session>>(this);

		// setting onOpen event handler
		processorSessionPtr->getResource()->setOpenCallback([this](Session* sessionPtr)
		{
			LOG(DEBUG) << LABELS{"cli"} << "Open session callback started (id=" << sessionPtr << ", stopping=" << stopping << ", sessions=" << sessions.size() << ")";

			// registering a new session if capacity allows so new requests can be accepted
			if (sessions.size() < maxSessions)
			{
				sessions.push_back(std::move(createSession()));
				LOG(DEBUG) << LABELS{"cli"} << "Session was added (id=" << this << ", sessions=" << sessions.size() << ")";
			} else {
				LOG(DEBUG) << LABELS{"cli"} << "Limit of active sessions was reached (id=" << this << ", sessions=" << sessions.size() << " max=" << maxSessions << ")";
				stopAcceptor();
			}

			LOG(DEBUG) << LABELS{"cli"} << "Open session callback completed (id=" << sessionPtr << ", stopping=" << stopping << ", sessions=" << sessions.size() << ")";
		});

		// setting onClose event handler
		processorSessionPtr->getResource()->setCloseCallback([this](Session* sessionPtr, const std::error_code& error)
		{
			LOG(DEBUG) << LABELS{"cli"} << "Close session callback started (id=" << sessionPtr << ", stopping=" << stopping << ", sessions=" << sessions.size() << ")";

			// session cannot be deleted at this moment as this method is called withing this session
			processorProxyPtr->process([this, sessionPtr]
			{
				// by the time this handler is processed, session might be deleted however sessionPtr is good enough for comparison
				deleteSession(sessionPtr);

				// starting acceptor if required
				if (!acceptorPtr) {
					startAcceptor();
				}
			});

			LOG(DEBUG) << LABELS{"cli"} << "Close session callback completed (id=" << sessionPtr << ", stopping=" << stopping << ", sessions=" << sessions.size() << ")";
		});

		// start listening to the incoming requests
		processorSessionPtr->getResource()->open();

		LOG(DEBUG) << LABELS{"cli"} << "New session was created (id=" << processorSessionPtr->getResource() << ")";
		return std::move(processorSessionPtr);
	}
*/
/*
	void Server::deleteSession(Session* sessionPtr)
	{
		// removing session from the vector
		sessions.erase(
			std::remove_if(
				sessions.begin(),
				sessions.end(),
				[&](auto& processorSessionPtr) -> bool
				{
					return sessionPtr == processorSessionPtr->getResource();
				}
			),
			sessions.end()
		);
		LOG(DEBUG) << LABELS{"cli"} << "Session was removed (id=" << this << ", sessions=" << sessions.size() << ")";
	}
*/

	void Server::setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerBase>* p)
	{
		processorProxyPtr = p;
	}


	void Server::start()
	{
		LOG(INFO) << LABELS{"slim"} << "Starting new server (id=" << this << ", port=" << port << ", max sessions=" << maxSessions << ")...";

		// start accepting new sessions
		startAcceptor();

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

			// creating native socket object
			asio::ip::tcp::socket socket{*processorProxyPtr->getDispatcher()};

			// creating new session used for accepting connections
			sessions.emplace_back(std::move(socket), *acceptorPtr, [](const std::error_code&) {});

			LOG(DEBUG) << LABELS{"slim"} << "Acceptor was started (id=" << acceptorPtr.get() << ")";
		}
	}


	void Server::stop()
	{
		LOG(INFO) << LABELS{"slim"} << "Stopping server...";

		// stop accepting any new connections
		stopAcceptor();

		// closing active sessions; sessions will be deleted by default Session's destructor
/*
		for (auto& sessionProcessorPtr : sessions)
		{
			sessionProcessorPtr->getResource()->close();
		}
*/
		LOG(INFO) << LABELS{"slim"} << "Server was stopped (id=" << this << ")";
	}


	void Server::stopAcceptor() {

		// disposing acceptor to prevent from new incomming requests
		if (acceptorPtr) {
			LOG(DEBUG) << LABELS{"slim"} << "Stopping acceptor (id=" << acceptorPtr.get() << ")...";

			acceptorPtr->cancel();
			acceptorPtr->close();

			LOG(DEBUG) << LABELS{"slim"} << "Acceptor was stopped (id=" << acceptorPtr.get() << ")";
			acceptorPtr.reset();
		}
	}
}
