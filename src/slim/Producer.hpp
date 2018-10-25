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

#include "slim/Chunk.hpp"
#include "slim/ContainerBase.hpp"


namespace slim
{
	namespace ts = type_safe;

	class Producer
	{
		public:
			Producer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> p)
			: processorProxy{p} {}

			virtual ~Producer() = default;

			virtual conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> getProcessorProxy()
			{
				return processorProxy;
			}

			virtual bool                       isRunning() = 0;
			virtual ts::optional<unsigned int> produceChunk(std::function<bool(Chunk&)>& consumer) = 0;
			virtual ts::optional<unsigned int> skipChunk() = 0;
			virtual void                       start() = 0;
			virtual void                       stop(bool gracefully = true) = 0;

		private:
			conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
	};
}
