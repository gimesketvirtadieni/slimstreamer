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


namespace slim
{
	namespace proto
	{
		template<typename CommandType>
		class InboundCommand
		{
			protected:
				using BufferType = util::RawBuffer<std::uint8_t>;

				union SizeElement
				{
					std::uint8_t  array[4];
					std::uint32_t size;
				};

			public:
				template<typename CommandBufferType>
				InboundCommand(const BufferType::CapacityType& bufferSize, const CommandBufferType& commandBuffer, const std::string& label)
				: buffer{bufferSize}
				{
					// validating provided data is sufficient for the fixed part of the command
					if (sizeof(CommandType) > commandBuffer.getSize())
					{
						throw slim::Exception("Message is too small for the fixed part of the command");
					}

					// making sure there is enough data provided for the dynamic part of the command
					if (!isEnoughData(commandBuffer))
					{
						throw slim::Exception("Message is too small for CommandType command");
					}

					// serializing fixed part of the command
					for (std::size_t i{0}; i < sizeof(CommandType); i++)
					{
						buffer.getBuffer()[i] = commandBuffer[i];
					}

					// TODO: refactor
					auto* commandPtr{(CommandType*)buffer.getBuffer()};

					// converting command size data
					commandPtr->size = ntohl(commandPtr->size);

					// validating length attribute from the command (last byte accounts for tailing zero)
					auto messageSize{commandPtr->size + sizeof(commandPtr->opcode) + sizeof(commandPtr->size)};
					if (messageSize >= buffer.getSize())
					{
						throw slim::Exception("Length provided in CommandType command is too big");
					}

					// serializing dynamic part of the command
					for (std::size_t i{sizeof(CommandType) - 1}; i < messageSize; i++)
					{
						buffer.getBuffer()[i] = commandBuffer[i];
					}
					buffer.getBuffer()[messageSize] = 0;

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
					return (CommandType*)buffer.getBuffer();
				}

				auto getSize()
				{
					return getData()->size;
				}

				template <typename BufferType>
				inline static auto isEnoughData(const BufferType& buffer)
				{
					auto result{false};

					// TODO: consider max length size
					auto indexOffset{typename BufferType::IndexType{4}};
					if (indexOffset + sizeof(SizeElement) <= buffer.getSize())
					{
						SizeElement sizeElement;
						for (auto i{indexOffset}; i < sizeof(SizeElement); i++)
						{
							sizeElement.array[i] = buffer[i];
						}

						// there is enough data in the buffer if provided length is less of equal to the buffer size
						result = (indexOffset + sizeof(SizeElement) + ntohl(sizeElement.size) <= buffer.getSize());
					}

					return result;
				}
/*
				inline static auto isEnoughData(unsigned char* buffer, std::size_t size)
				{
					auto        result{false};
					std::size_t offset{4};

					// TODO: consider max length size
					if (size >= offset + sizeof(std::uint32_t))
					{
						std::uint32_t length{ntohl(*(std::uint32_t*)(buffer + offset))};
						result = (size >= (length + offset + sizeof(std::uint32_t)));
					}

					return result;
				}
*/
			protected:
				InboundCommand() = default;

				inline auto& getBuffer()
				{
					return buffer;
				}

			private:
				BufferType buffer;
		};
	}
}
