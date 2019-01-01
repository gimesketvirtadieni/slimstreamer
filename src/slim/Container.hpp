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

#include <conwrap2/ProcessorProxy.hpp>
#include <functional>
#include <memory>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template<typename CommandServerType, typename StreamingServerType, typename DiscoveryServerType, typename SchedulerType>
	class Container : public ContainerBase
	{
		public:
			Container(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy, std::unique_ptr<CommandServerType> cse, std::unique_ptr<StreamingServerType> sse, std::unique_ptr<DiscoveryServerType> dse, std::unique_ptr<SchedulerType> sc)
			: ContainerBase{processorProxy}
			, commandServerPtr{std::move(cse)}
			, streamingServerPtr{std::move(sse)}
			, discoveryServerPtr{std::move(dse)}
			, schedulerPtr{std::move(sc)} {}

			// using Rule Of Zero
			virtual ~Container() = default;
			Container(const Container&) = delete;             // non-copyable
			Container& operator=(const Container&) = delete;  // non-assignable
			Container(Container&& rhs) = default;
			Container& operator=(Container&& rhs) = default;

			virtual bool isSchedulerRunning() override
			{
				return schedulerPtr->isRunning();
			}

			virtual void start() override
			{
				commandServerPtr->start();
				streamingServerPtr->start();
				discoveryServerPtr->start();
				schedulerPtr->start();
			}

			virtual void stop() override
			{
				schedulerPtr->stop();
				discoveryServerPtr->stop();
				streamingServerPtr->stop();
				commandServerPtr->stop();
			}

		private:
			std::unique_ptr<CommandServerType>   commandServerPtr;
			std::unique_ptr<StreamingServerType> streamingServerPtr;
			std::unique_ptr<DiscoveryServerType> discoveryServerPtr;
			std::unique_ptr<SchedulerType>       schedulerPtr;
	};
}
