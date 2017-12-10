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

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template<typename Server, typename Streamer>
	class Container : public ContainerBase
	{
		public:
			Container(Server se, Streamer st)
			: server{std::move(se)}
			, streamer{std::move(st)} {}

			// using Rule Of Zero
			virtual ~Container() = default;
			Container(const Container&) = delete;             // non-copyable
			Container& operator=(const Container&) = delete;  // non-assignable
			Container(Container&& rhs) = default;
			Container& operator=(Container&& rhs) = default;

			virtual void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerBase>* p)
			{
				ContainerBase::setProcessorProxy(p);
				server.setProcessorProxy(p);
				streamer.setProcessorProxy(p);
			}

			void start()
			{
				server.start();
				streamer.start();
			}

			void stop()
			{
				streamer.stop();
				server.stop();
			}

		private:
			Server   server;
			Streamer streamer;
	};
}
