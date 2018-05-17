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


namespace slim
{
	namespace conn
	{
		template<typename ConnectionType>
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

				// TODO: reconsider, using getters like this is not 'nice'
				inline auto& getCloseCallback()
				{
					return closeCallback;
				}

				inline auto& getDataCallback()
				{
					return dataCallback;
				}

				inline auto& getOpenCallback()
				{
					return openCallback;
				}

				inline auto& getStartCallback()
				{
					return startCallback;
				}

				inline auto& getStopCallback()
				{
					return stopCallback;
				}

				inline auto& setCloseCallback(std::function<void(ConnectionType&)> c)
				{
					if (c)
					{
						closeCallback = std::move(c);
					}
					else
					{
						closeCallback = [](auto& connection) {};
					}
					return (*this);
				}

				inline auto& setDataCallback(std::function<void(ConnectionType&, unsigned char*, const std::size_t)> c)
				{
					if (c)
					{
						dataCallback = std::move(c);
					}
					else
					{
						dataCallback = [](auto& connection, unsigned char* buffer, const std::size_t size) {};
					}
					return (*this);
				}

				inline auto& setOpenCallback(std::function<void(ConnectionType&)> c)
				{
					if (c)
					{
						openCallback = std::move(c);
					}
					else
					{
						openCallback = [](auto& connection) {};
					}
					return (*this);
				}

				inline auto& setStartCallback(std::function<void(ConnectionType&)> c)
				{
					if (c)
					{
						startCallback = std::move(c);
					}
					else
					{
						startCallback = [](auto& connection) {};
					}
					return (*this);
				}

				inline auto& setStopCallback(std::function<void(ConnectionType&)> c)
				{
					if (c)
					{
						stopCallback = std::move(c);
					}
					else
					{
						stopCallback = [](auto& connection) {};
					}
					return (*this);
				}

			private:
				std::function<void(ConnectionType&)>                                    startCallback{[](auto& connection) {}};
				std::function<void(ConnectionType&)>                                    openCallback{[](auto& connection) {}};
				std::function<void(ConnectionType&, unsigned char*, const std::size_t)> dataCallback{[](auto& connection, auto* buffer, auto size) {}};
				std::function<void(ConnectionType&)>                                    closeCallback{[](auto& connection) {}};
				std::function<void(ConnectionType&)>                                    stopCallback{[](auto& connection) {}};
		};

		template<>
		class CallbacksBase<void>
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

				inline auto& setDataCallback(std::function<void(unsigned char*, const std::size_t)> c)
				{
					if (c)
					{
						dataCallback = std::move(c);
					}
					else
					{
						dataCallback = [](unsigned char* buffer, const std::size_t size) {};
					}
					return (*this);
				}

				inline auto& setStartCallback(std::function<void()> c)
				{
					if (c)
					{
						startCallback = std::move(c);
					}
					else
					{
						startCallback = [] {};
					}
					return (*this);
				}

				inline auto& setStopCallback(std::function<void()> c)
				{
					if (c)
					{
						stopCallback = std::move(c);
					}
					else
					{
						stopCallback = [] {};
					}
					return (*this);
				}

			private:
				std::function<void()>                                  startCallback{[] {}};
				std::function<void(unsigned char*, const std::size_t)> dataCallback{[](auto* buffer, auto size) {}};
				std::function<void()>                                  stopCallback{[]() {}};
		};
	}
}
