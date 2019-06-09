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
	namespace proto
	{
		enum class CommandSelection : char
		{
			Start   = 's',
			Stop    = 'q',
			Time    = 't',
			Pause   = 'p',
			Unpause = 'u',
		};

		enum class FormatSelection
		{
			PCM,
			FLAC,
		};

		template<typename CommandType>
		class OutboundCommand
		{
			public:
 				// using Rule Of Zero
				virtual ~OutboundCommand() = default;
				OutboundCommand(const OutboundCommand&) = default;
				OutboundCommand& operator=(const OutboundCommand&) = default;
				OutboundCommand(OutboundCommand&& rhs) = default;
				OutboundCommand& operator=(OutboundCommand&& rhs) = default;

				virtual CommandType* getBuffer() = 0;
				virtual std::size_t  getSize()   = 0;

			protected:
				union SizeElement
				{
					std::uint8_t  array[4];
					std::uint32_t size;
				};

				OutboundCommand() = default;
		};
	}
}
