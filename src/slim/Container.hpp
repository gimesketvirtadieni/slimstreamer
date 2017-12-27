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
	template<typename SchedulerType, typename CommandServerType>
	class Container : public ContainerBase
	{
		public:
			Container(std::unique_ptr<SchedulerType> sc, std::unique_ptr<CommandServerType> cse)
			: schedulerPtr{std::move(sc)}
			, commandServerPtr{std::move(cse)} {}

			// using Rule Of Zero
			virtual ~Container() = default;
			Container(const Container&) = delete;             // non-copyable
			Container& operator=(const Container&) = delete;  // non-assignable
			Container(Container&& rhs) = default;
			Container& operator=(Container&& rhs) = default;

			// virtualization is required as Processor uses std::unique_ptr<ContainerBase>
			virtual void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerBase>* p)
			{
				ContainerBase::setProcessorProxy(p);
				commandServerPtr->setProcessorProxy(p);
				schedulerPtr->setProcessorProxy(p);
			}

			virtual void start() override
			{
				commandServerPtr->start();
				schedulerPtr->start();
			}

			virtual void stop() override
			{
				schedulerPtr->stop();
				commandServerPtr->stop();
			}

		private:
			std::unique_ptr<SchedulerType>     schedulerPtr;
			std::unique_ptr<CommandServerType> commandServerPtr;
	};
}
