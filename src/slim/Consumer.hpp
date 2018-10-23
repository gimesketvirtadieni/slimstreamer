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
#include <memory>

#include "slim/Chunk.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	class Consumer
	{
		public:
			Consumer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> p, unsigned int s)
			: processorProxy{p}
			, samplingRate{s} {}

			virtual     ~Consumer() = default;
			virtual bool consumeChunk(Chunk&) = 0;

			virtual conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> getProcessorProxy()
			{
				return processorProxy;
			}

			virtual unsigned int getSamplingRate() const
			{
				return samplingRate;
			}

			virtual void setSamplingRate(unsigned int s)
			{
				samplingRate = s;
			}

			virtual void start() = 0;
			virtual void stop(bool gracefully = true) = 0;

		private:
			conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
			unsigned int                                             samplingRate;
	};
}
