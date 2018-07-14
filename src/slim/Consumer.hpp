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

#include <conwrap/ProcessorProxy.hpp>

#include "slim/Chunk.hpp"
#include "slim/ContainerBase.hpp"


namespace slim
{
	class Consumer
	{
		public:
			virtual                                        ~Consumer() = default;
			virtual bool                                    consume(Chunk&) = 0;
			virtual conwrap::ProcessorProxy<ContainerBase>* getProcessorProxy()
			{
				return processorProxyPtr;
			}

			virtual void setProcessorProxy(conwrap::ProcessorProxy<ContainerBase>* p)
			{
				processorProxyPtr = p;
			}

			virtual void start() = 0;
			virtual void stop(bool gracefully = true) = 0;

		private:
			conwrap::ProcessorProxy<ContainerBase>* processorProxyPtr{nullptr};
	};
}
