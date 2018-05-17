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


namespace slim
{
	namespace conn
	{
		// forward declaration required to prevent loops in including headers
		template <typename ContainerType>
		class UDPServer;

		template<typename ContainerType>
		class UDPCallbacks
		{
			public:
				UDPCallbacks() = default;

				// using Rule Of Zero
			   ~UDPCallbacks() = default;
				UDPCallbacks(const UDPCallbacks&) = delete;             // non-copyable
				UDPCallbacks& operator=(const UDPCallbacks&) = delete;  // non-assignable
				UDPCallbacks(UDPCallbacks&& rhs) = default;
				UDPCallbacks& operator=(UDPCallbacks&& rhs) = default;

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

				inline auto& setDataCallback(std::function<void(UDPServer<ContainerType>&, unsigned char*, const std::size_t)> c)
				{
					if (c)
					{
						dataCallback = std::move(c);
					}
					else
					{
						dataCallback = [](UDPServer<ContainerType>&, unsigned char* buffer, const std::size_t size) {};
					}
					return (*this);
				}

				inline auto& setStartCallback(std::function<void(UDPServer<ContainerType>&)> c)
				{
					if (c)
					{
						startCallback = std::move(c);
					}
					else
					{
						startCallback = [](UDPServer<ContainerType>&) {};
					}
					return (*this);
				}

				inline auto& setStopCallback(std::function<void(UDPServer<ContainerType>&)> c)
				{
					if (c)
					{
						stopCallback = std::move(c);
					}
					else
					{
						stopCallback = [](UDPServer<ContainerType>&) {};
					}
					return (*this);
				}

			private:
				std::function<void(UDPServer<ContainerType>&)>                                    startCallback{[](auto& server) {}};
				std::function<void(UDPServer<ContainerType>&, unsigned char*, const std::size_t)> dataCallback{[](auto& server, auto* buffer, auto size) {}};
				std::function<void(UDPServer<ContainerType>&)>                                    stopCallback{[](auto& server) {}};
		};
	}
}
