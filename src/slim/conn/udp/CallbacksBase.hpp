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

#include <cstddef>  // std::size_t
#include <functional>


namespace slim
{
	namespace conn
	{
		namespace udp
		{
			template<typename ServerType>
			class CallbacksBase
			{
				public:
					CallbacksBase() = default;

					// using Rule Of Zero
				   ~CallbacksBase() = default;
					CallbacksBase(const CallbacksBase&) = delete;             // non-copyable
					CallbacksBase& operator=(const CallbacksBase&) = delete;  // non-assignable
					CallbacksBase(CallbacksBase&& rhs) = default;
					CallbacksBase& operator=(CallbacksBase&& rhs) = default;

					inline auto& getDataCallback()
					{
						return dataCallback;
					}

					inline auto& getStartCallback()
					{
						return startCallback;
					}

					inline auto& getStopCallback()
					{
						return stopCallback;
					}

					inline auto& setDataCallback(std::function<void(ServerType&, unsigned char*, const std::size_t)> c)
					{
						if (c)
						{
							dataCallback = std::move(c);
						}
						else
						{
							dataCallback = [](ServerType&, unsigned char* buffer, const std::size_t size) {};
						}
						return (*this);
					}

					inline auto& setStartCallback(std::function<void(ServerType&)> c)
					{
						if (c)
						{
							startCallback = std::move(c);
						}
						else
						{
							startCallback = [](ServerType&) {};
						}
						return (*this);
					}

					inline auto& setStopCallback(std::function<void(ServerType&)> c)
					{
						if (c)
						{
							stopCallback = std::move(c);
						}
						else
						{
							stopCallback = [](ServerType&) {};
						}
						return (*this);
					}

				private:
					std::function<void(ServerType&)>                                    startCallback{[](auto& server) {}};
					std::function<void(ServerType&, unsigned char*, const std::size_t)> dataCallback{[](auto& server, auto* buffer, auto size) {}};
					std::function<void(ServerType&)>                                    stopCallback{[](auto& server) {}};
			};
		}
	}
}
