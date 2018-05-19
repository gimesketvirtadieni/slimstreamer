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

#include "slim/conn/udp/CallbacksBase.hpp"
#include "slim/conn/udp/Server.hpp"


namespace slim
{
	namespace conn
	{
		namespace udp
		{
			template<typename ContainerType>
			using Callbacks = CallbacksBase<Server<ContainerType>>;
		}
	}
}
