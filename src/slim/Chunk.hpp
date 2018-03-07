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

#include "slim/util/ExpandableBuffer.hpp"


namespace slim
{
	class Chunk
	{
		public:
			// using Rule Of Zero
			Chunk() = default;
		   ~Chunk() = default;

			Chunk(const Chunk& rhs) = default;
			Chunk& operator=(const Chunk& rhs) = default;
			Chunk(Chunk&& rhs) = default;
			Chunk& operator=(Chunk&& rhs) = default;

			inline auto& getBuffer()
			{
				return buffer;
			}

		private:
			util::ExpandableBuffer buffer;
	};


	// TODO: implement
	//inline std::ostream &operator<<(std::ostream &outputstream, Chunk const &chunk)
	//{
	//	return outputstream << ""/*m.i*/;
	//}
}
