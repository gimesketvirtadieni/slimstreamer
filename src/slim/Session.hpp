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

#include <conwrap/ProcessorAsioProxy.hpp>
#include <functional>
#include <memory>

#include "slim/ContainerBase.hpp"


namespace slim
{
	class Server;

	class Session
	{
		public:
			Session(conwrap::ProcessorAsioProxy<ContainerBase>* p, std::function<void(Session&)> stac, std::function<void(Session&)> oc, std::function<void(Session&)> cc, std::function<void(Session&)> stoc)
			: processorProxyPtr{p}
			, nativeSocket{*processorProxyPtr->getDispatcher()}
			, startCallback{std::move(stac)}
			, openCallback{std::move(oc)}
			, closeCallback{std::move(cc)}
			, stopCallback{std::move(stoc)}
			, opened{false} {}

			~Session()
			{
				try
				{
					stop();
				}
				catch(...) {}
			}

			Session(const Session&) = delete;             // non-copyable
			Session& operator=(const Session&) = delete;  // non-assignable
			Session(Session&& rhs) = delete;              // non-movable
			Session& operator=(Session&& rhs) = delete;   // non-movable-assignable

			void start(asio::ip::tcp::acceptor& acceptor);
			void stop();

		protected:
			void onClose(const std::error_code error);
			void onData(const std::error_code error, const std::size_t receivedSize);
			void onOpen(const std::error_code error);
			void onStop();

		private:
			conwrap::ProcessorAsioProxy<ContainerBase>* processorProxyPtr;
			asio::ip::tcp::socket                       nativeSocket;
			std::function<void(Session&)>               startCallback;
			std::function<void(Session&)>               openCallback;
			std::function<void(Session&)>               closeCallback;
			std::function<void(Session&)>               stopCallback;
			bool                                        opened;
			// TODO: work in progress
			unsigned char                               data[1000];
	};
}
