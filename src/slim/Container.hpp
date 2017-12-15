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
#include <memory>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template<typename Server, typename Streamer>
	class Container : public ContainerBase
	{
		public:
			Container(std::unique_ptr<Server> se, std::unique_ptr<Streamer> st)
			: serverPtr{std::move(se)}
			, streamerPtr{std::move(st)} {}

			// using Rule Of Zero
			virtual ~Container() = default;
			Container(const Container&) = delete;             // non-copyable
			Container& operator=(const Container&) = delete;  // non-assignable
			Container(Container&& rhs) = default;
			Container& operator=(Container&& rhs) = default;

			virtual void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerBase>* p)
			{
				ContainerBase::setProcessorProxy(p);
				serverPtr->setProcessorProxy(p);
				streamerPtr->setProcessorProxy(p);
			}

			void start()
			{
				serverPtr->start();
				streamerPtr->start();
			}

			void stop()
			{
				streamerPtr->stop();
				serverPtr->stop();
			}

		private:
			std::unique_ptr<Server>   serverPtr;
			std::unique_ptr<Streamer> streamerPtr;
	};
}
