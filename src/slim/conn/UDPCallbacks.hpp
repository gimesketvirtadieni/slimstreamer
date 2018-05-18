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

#include "slim/conn/UDPCallbacksBase.hpp"
#include "slim/conn/UDPServer.hpp"


namespace slim
{
	namespace conn
	{
		template<typename ContainerType>
		using UDPCallbacks = UDPCallbacksBase<UDPServer<ContainerType>>;
	}
}
