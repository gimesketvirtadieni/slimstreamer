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
				CallbacksBase(std::function<void(ConnectionType&)> stac, std::function<void(ConnectionType&)> oc, std::function<void(ConnectionType&, unsigned char*, const std::size_t)> dc, std::function<void(ConnectionType&)> cc, std::function<void(ConnectionType&)> stoc)
				: startCallback{std::move(stac)}
				, openCallback{std::move(oc)}
				, dataCallback{std::move(dc)}
				, closeCallback{std::move(cc)}
				, stopCallback{std::move(stoc)} {}

				// using Rule Of Zero
			   ~CallbacksBase() = default;
				CallbacksBase(const CallbacksBase&) = delete;             // non-copyable
				CallbacksBase& operator=(const CallbacksBase&) = delete;  // non-assignable
				CallbacksBase(CallbacksBase&& rhs) = default;
				CallbacksBase& operator=(CallbacksBase&& rhs) = default;

			//private:
				std::function<void(ConnectionType&)>                                    startCallback;
				std::function<void(ConnectionType&)>                                    openCallback;
				std::function<void(ConnectionType&, unsigned char*, const std::size_t)> dataCallback;
				std::function<void(ConnectionType&)>                                    closeCallback;
				std::function<void(ConnectionType&)>                                    stopCallback;
		};
	}
}
