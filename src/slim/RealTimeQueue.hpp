#pragma once

#include <spsc-bounded-queue.hpp>


namespace slim
{
	template<typename T>
	using RealTimeQueue = spsc_bounded_queue_t<T>;
}
