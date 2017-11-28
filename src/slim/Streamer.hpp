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
#include <mutex>
#include <thread>
#include <vector>

#include "slim/Pipeline.hpp"


namespace slim
{
	class Streamer
	{
		public:
			explicit Streamer(std::vector<Pipeline> pipelines);

			// using Rule Of Zero
			~Streamer() = default;
			Streamer(const Streamer&) = delete;             // non-copyable
			Streamer& operator=(const Streamer&) = delete;  // non-assignable
			Streamer(Streamer&& rhs) = default;
			Streamer& operator=(Streamer&& rhs) = default;

			void setProcessorProxy(conwrap::ProcessorProxy<Streamer>* p);
			void start();
			void stop(bool gracefully = true);

		protected:
			void stream();

		private:
			std::vector<Pipeline>              pipelines;
			std::vector<std::thread>           threads;
			conwrap::ProcessorProxy<Streamer>* processorProxyPtr;
			std::mutex                         lock;
	};
}
