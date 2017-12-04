// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

// Note:
// A combination of the algorithms described by the circular buffers
// documentation found in the Linux kernel, and the bounded MPMC queue
// by Dmitry Vyukov[1]. Implemented in pure C++11. Should work across
// most CPU architectures.
//
// [1] http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

#pragma once

#include <atomic>
#include <assert.h>
#include <cstddef>
#include <functional>
#include <type_traits>


/*
 * This code is based on https://github.com/mstump/queues/blob/master/include/spsc-bounded-queue.hpp
 * by Dmitry Vyukov. To make this queue suitable for real-time safe code, there are few
 * customizations:
 *  - added <cstddef> include required for size_t type
 *  - added an initializer for buffer elements
 *  - changed API to allow using non-copyable & non-movable T
 */
namespace slim
{
	template<typename T>
	class RealTimeQueue
	{
		public:
			RealTimeQueue(size_t size, std::function<void(T&)> initializer = [](T&) {})
			: _size{size}
			, _mask{_size - 1}
			, _buffer{reinterpret_cast<T*>(new aligned_t[size + 1])}  // need one extra element for a guard
			, _head{0}
			, _tail{0}
			{
				// make sure it's a power of 2
				// TODO: get rid of this assert
				assert((_size != 0) && ((_size & (~_size + 1)) == _size));

				// invoking constructors explicitly as 'new aligned_t[size + 1]' does not do that
				new (_buffer) T[_size]();

				// initializing all elements
				for(size_t i = 0; i < _size; i++)
				{
					initializer(_buffer[i]);
				}
			}

			~RealTimeQueue()
			{
				// invoking destructors explicitly
				for(size_t i = 0; i < _size; i++)
				{
					_buffer[i].~T();
				}

				// releasing memory; preventing from calling destructors
				delete[] (aligned_t*)_buffer;
			}

			RealTimeQueue(const RealTimeQueue&) = delete;             // non-copyable
			RealTimeQueue(const RealTimeQueue&&) = delete;            // non-movable
			RealTimeQueue& operator=(const RealTimeQueue&) = delete;  // non-assignable
			RealTimeQueue& operator=(RealTimeQueue&&) = delete;       // non-movable

			template<typename M, typename H = std::function<void()>>
			void enqueue(M mover, H overflowHandler = [] {})
			{
				const size_t head = _head.load(std::memory_order_relaxed);

				if (((_tail.load(std::memory_order_acquire) - (head + 1)) & _mask) >= 1) {
					mover(_buffer[head & _mask]);
					_head.store(head + 1, std::memory_order_release);
				} else {
					overflowHandler();
				}
			}

			template<typename M, typename H = std::function<void()>>
			void dequeue(M mover, H underfowHandler = [] {})
			{
				const size_t tail = _tail.load(std::memory_order_relaxed);

				if (((_head.load(std::memory_order_acquire) - tail) & _mask) >= 1) {
					mover(_buffer[_tail & _mask]);
					_tail.store(tail + 1, std::memory_order_release);
				} else {
					underfowHandler();
				}
			}

		private:
			typedef typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type aligned_t;
			typedef char cache_line_pad_t[64];

			cache_line_pad_t    _pad0;
			const size_t        _size;
			const size_t        _mask;
			T* const            _buffer;

			cache_line_pad_t    _pad1;
			std::atomic<size_t> _head;

			cache_line_pad_t    _pad2;
			std::atomic<size_t> _tail;
	};
}
