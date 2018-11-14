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
#include <type_safe/optional.hpp>

#include "slim/Chunk.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	namespace ts = type_safe;

	class Consumer
	{
		public:
			Consumer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> p)
			: processorProxy{p} {}

			virtual     ~Consumer() = default;
			virtual bool consumeChunk(Chunk&) = 0;

			virtual conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> getProcessorProxy()
			{
				return processorProxy;
			}

			virtual ts::optional<unsigned int> getSamplingRate() const
			{
				return samplingRate;
			}

			virtual void setSamplingRate(ts::optional<unsigned int> s)
			{
				samplingRate = s.map([&](auto& samplingRate)
				{
					auto result{ts::optional<unsigned int>{ts::nullopt}};

					if (samplingRate)
					{
						result = samplingRate;
					}

					return result;
				});
			}

			virtual void start() = 0;
			virtual void stop(bool gracefully = true) = 0;

		private:
			conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
			ts::optional<unsigned int>                               samplingRate{ts::nullopt};
	};
}
