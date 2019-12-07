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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "slim/util/buffer/Array.hpp"
#include "slim/util/buffer/Helper.hpp"


TEST(Helper, EmptyStringTest1)
{
	std::string str = "";
    std::stringstream ss;

    slim::util::buffer::writeToStream(str, str.size(), ss);

    EXPECT_EQ(ss.str(), "");
}

TEST(Helper, OneRowTest1)
{
	std::string str = "1234567890";
    std::stringstream ss;

    slim::util::buffer::writeToStream(str, str.size(), ss);

    EXPECT_EQ(ss.str(), "00 | 31 32 33 34 35 36 37 38 39 30                   | 1234567890");
}

TEST(Helper, OneRowTest2)
{
	std::string str = "1234567890123456";
    std::stringstream ss;

    slim::util::buffer::writeToStream(str, str.size(), ss);

    EXPECT_EQ(ss.str(), "00 | 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 | 1234567890123456");
}

TEST(Helper, TwoRowsTest1)
{
	std::string str = "12345678901234561234567890";
    std::stringstream ss;

    slim::util::buffer::writeToStream(str, str.size(), ss);

    EXPECT_EQ(ss.str(), "00 | 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 | 1234567890123456\n01 | 31 32 33 34 35 36 37 38 39 30                   | 1234567890");
}

TEST(Helper, TwoRowsTest2)
{
	std::string str = "12345678901234561234567890123456";
    std::stringstream ss;

    slim::util::buffer::writeToStream(str, str.size(), ss);

    EXPECT_EQ(ss.str(), "00 | 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 | 1234567890123456\n01 | 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 | 1234567890123456");
}

TEST(Helper, SpecialCharsTest1)
{
	slim::util::buffer::Array<char> chars{8};
    std::stringstream ss;

    chars[0] = '1';
    chars[1] = '2';
    chars[2] = '3';
    chars[3] = '\r';
    chars[4] = '\n';
    chars[5] = '\t';
    chars[6] = '\x0';
    chars[7] = '\xFF';

    slim::util::buffer::writeToStream(chars, chars.getSize(), ss);

    EXPECT_EQ(ss.str(), "00 | 31 32 33 0d 0a 09 00 ff                         | 123.....");
}

TEST(Helper, ManyLines1)
{
	std::string str;
    std::stringstream ss1;
    std::stringstream ss2;
    auto totalRows = 300u;

    for (auto r = 0u; r < totalRows; r++)
    {
        str.append("1234567890123456");

        ss2 << std::hex << std::setfill('0') << std::setw(2) << (r & 0xFF) << " | 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 | 1234567890123456";
        if (r < totalRows - 1)
        {
            ss2 << std::endl;
        }
    }

    slim::util::buffer::writeToStream(str, str.size(), ss1);

    EXPECT_EQ(ss1.str(), ss2.str());
}
