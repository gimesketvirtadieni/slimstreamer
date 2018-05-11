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

#include <functional>

#include "slim/Consumer.hpp"


namespace slim
{
	class Producer
	{
		public:
			virtual     ~Producer() = default;
			virtual bool isAvailable() = 0;
			virtual bool isRunning() = 0;
			virtual void pause(unsigned int millisec) = 0;
			virtual bool produce(Consumer&) = 0;
			virtual void start(std::function<void()> = [] {}) = 0;
			virtual void stop(bool gracefully = true) = 0;
	};
}
