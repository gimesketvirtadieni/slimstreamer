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
#include "slim/Session.hpp"


namespace slim
{
	void Session::onClose(const std::error_code error)
	{
		LOG(DEBUG) << LABELS{"slim"} << "Closing session (id=" << this << ", error='" << error.message() << "')...";

		// invoking close callback before session is desposed and setting opened status
		if (opened && closeCallback)
		{
			closeCallback(*this);
		}
		opened = false;

		// stopping this session after it's been closed
		onStop();

		LOG(DEBUG) << LABELS{"slim"} << "Session was closed (id=" << this << ")";
	}


	void Session::onData(const std::error_code error, const std::size_t receivedSize)
	{
		// if error then closing this session
		if (error)
		{
			onClose(error);
		}
		else
		{
			// any processing should be done here
			// ...

			// submitting a new task here allows other tasks to progress
			processorProxyPtr->process([&]
			{
				// keep receiving data 'recursivelly' (task processor is used instead of stack)
				nativeSocket.async_read_some(
					asio::buffer(data, 1000),
					[&](const std::error_code error, std::size_t bytes_transferred)
					{
						processorProxyPtr->wrap([=]
						{
							onData(error, bytes_transferred);
						})();
					}
				);
			});
		}
	}


	void Session::onOpen(const std::error_code error)
	{
		if (error)
		{
			onClose(error);
		}
		else
		{
			LOG(DEBUG) << LABELS{"slim"} << "Opening session (id=" << this << ")...";

			// invoking open callback and setting opened status
			if (!opened && openCallback)
			{
				openCallback(*this);
			}
			opened = true;

			// start receiving data
			onData(error, 0);

			LOG(DEBUG) << LABELS{"slim"} << "Session was opened (id=" << this << ")";
		}
	}


	void Session::onStop()
	{
		// invoking stop callback as a handler for processor, which will be execusted AFTER the close handler
		if (stopCallback)
		{
			processorProxyPtr->process([&]
			{
				stopCallback(*this);
			});
		}
	}


	void Session::start(asio::ip::tcp::acceptor& acceptor)
	{
		// invoking open callback before any connectivy action
		if (startCallback)
		{
			startCallback(*this);
		}

		// TODO: validate result???
		acceptor.async_accept(
			nativeSocket,
			[&](const std::error_code& error)
			{
				processorProxyPtr->wrap([=]
				{
					onOpen(error);
				})();
			}
		);
	}


	void Session::stop()
	{
		if (nativeSocket.is_open())
		{
			// it will trigger chain of callbacks
			nativeSocket.shutdown(asio::socket_base::shutdown_both);
			nativeSocket.close();
		}
	}
}
