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
#include <cstdint>  // std::u..._t types

#include "slim/util/buffer/HeapBuffer.hpp"
#include "slim/util/buffer/Ring.hpp"


namespace slim
{
	namespace proto
	{
		template<typename CommandType>
		class InboundCommand
		{
			protected:
				union SizeElement
				{
					std::uint8_t  array[4];
					std::uint32_t size;
				};

			public:
				InboundCommand(const util::buffer::HeapBuffer<std::uint8_t>::SizeType& bufferSize, const util::buffer::Ring<std::uint8_t>& commandRingBuffer, const std::string& label)
				// TODO: buffer size must be calculated based on size provided in the command
				: buffer{commandRingBuffer.getCapacity()}
				{
					// validating provided data is sufficient for the fixed part of the command
					if (sizeof(CommandType) > commandRingBuffer.getSize())
					{
						throw slim::Exception("Message is too small for the fixed part of the command");
					}

					// making sure there is enough data provided for the dynamic part of the command
					if (!isEnoughData(commandRingBuffer))
					{
						throw slim::Exception("Message is too small for CommandType command");
					}

					// serializing fixed part of the command
					for (std::size_t i{0}; i < sizeof(CommandType); i++)
					{
						buffer.getData()[i] = commandRingBuffer[i];
					}

					// TODO: refactor
					auto* commandPtr{(CommandType*)buffer.getData()};

					// converting command size data
					commandPtr->size = ntohl(commandPtr->size);

					// validating length attribute from the command (last byte accounts for tailing zero)
					auto messageSize{commandPtr->size + sizeof(commandPtr->opcode) + sizeof(commandPtr->size)};
					if (messageSize > buffer.getSize())
					{
						throw slim::Exception("Length provided in CommandType command is too big");
					}

					// serializing dynamic part of the command
					for (std::size_t i{sizeof(CommandType)}; i < messageSize; i++)
					{
						buffer.getData()[i] = commandRingBuffer[i];
					}

					// validating there is command label present
					std::string s{commandPtr->opcode, sizeof(commandPtr->opcode)};
					if (label.compare(s))
					{
						throw slim::Exception("Missing 'TODO: <label>' label in the header");
					}

					getData()->convert();
				}

 				// using Rule Of Zero
				~InboundCommand() = default;
				InboundCommand(const InboundCommand&) = default;
				InboundCommand& operator=(const InboundCommand&) = default;
				InboundCommand(InboundCommand&& rhs) = default;
				InboundCommand& operator=(InboundCommand&& rhs) = default;

				auto* getData()
				{
					return (CommandType*)buffer.getData();
				}

				auto getSize()
				{
					// TODO: refactor
					return getData()->size + sizeof(SizeElement) + 4;
				}

				inline static auto isEnoughData(const util::buffer::Ring<std::uint8_t>& rinBuffer)
				{
					auto result{false};

					// TODO: consider max length size
					auto labelOffset{typename util::buffer::Ring<std::uint8_t>::IndexType{4}};
					if (labelOffset + sizeof(SizeElement) <= rinBuffer.getSize())
					{
						SizeElement sizeElement;
						for (auto i{std::size_t{0}}; i < sizeof(SizeElement); i++)
						{
							sizeElement.array[i] = rinBuffer[i + labelOffset];
						}

						// there is enough data in the buffer if provided length is less of equal to the buffer size
						result = (labelOffset + sizeof(sizeElement.size) + ntohl(sizeElement.size) <= rinBuffer.getSize());
					}

					return result;
				}

			protected:
				InboundCommand() = default;

				inline auto& getBuffer()
				{
					return buffer;
				}

			private:
				util::buffer::HeapBuffer<std::uint8_t> buffer;
		};
	}
}
