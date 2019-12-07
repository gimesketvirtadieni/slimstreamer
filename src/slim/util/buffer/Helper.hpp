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

#include <cstddef>   // std::size_t
#include <iomanip>   // std::setfill, std::setw
#include <iostream>  // std::ostream

namespace slim
{
namespace util
{
namespace buffer
{

template <typename BufferType>
void writeToStream(const BufferType& buffer, const std::size_t size, std::ostream& os)
{
	auto totalColumns = 16u;
	auto totalRows = std::size_t{size / totalColumns} + ((size % totalColumns) ? 1 : 0);

	for (std::size_t r = 0, i = 0; r < totalRows && i < size; r++, i += totalColumns)
	{
		auto charsLeft = size - i;
		auto columnsLeft = std::min(charsLeft, std::size_t{totalColumns});
		auto rowWriter = [&](auto charWriter)
		{
			for (std::uint8_t c = 0; c < columnsLeft; c++)
			{
				charWriter(buffer[i + c]);
			}
		};
		auto hexCharWriter = [&](char ch)
		{
			os << std::hex << std::setfill('0') << std::setw(2) << (0xFF & ch) << " ";
		};
		auto printableCharWriter = [&](char ch)
		{
			os << (std::isprint(static_cast<unsigned char>(ch)) ? ch : '.');
		};

		// write row number and separator
		os << std::hex << std::setfill('0') << std::setw(2) << (r & 0xFF) << " | ";

		// writing hex representation
		rowWriter(hexCharWriter);

		// writing separator between hex and chars
		if (charsLeft < totalColumns)
		{
			os << std::string((totalColumns - charsLeft) * 3, ' ');
		}
		os << "| ";

		// writing char representation
		rowWriter(printableCharWriter);

		// submitting EOL only if it is not the last row
		if (totalColumns < charsLeft)
		{
			os << std::endl;
		}
	}
}

}
}
}
