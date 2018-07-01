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

#include <chrono>

#include "slim/Consumer.hpp"
#include "slim/Producer.hpp"


namespace slim
{
	class Pipeline
	{
		using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

		public:
			Pipeline(std::reference_wrapper<Producer> p, std::reference_wrapper<Consumer> c)
			: producer{p}
			, consumer{c} {}

			// using Rule Of Zero
		   ~Pipeline() = default;
			Pipeline(const Pipeline&) = delete;             // non-copyable
			Pipeline& operator=(const Pipeline&) = delete;  // non-assignable
			Pipeline(Pipeline&& rhs) = default;
			Pipeline& operator=(Pipeline&& rhs) = default;

			inline auto& getConsumer()
			{
				return consumer.get();
			}

			inline auto& getProducer()
			{
				return producer.get();
			}

		private:
			std::reference_wrapper<Producer> producer;
			std::reference_wrapper<Consumer> consumer;
			std::optional<TimePoint>         pauseUntil{std::nullopt};
	};
}
