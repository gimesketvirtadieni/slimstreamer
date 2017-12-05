#include <gtest/gtest.h>
#include <iostream>


int main(int argc, char** argv) {

	// running all tests
	testing::InitGoogleTest(&argc, argv);
	int return_value = RUN_ALL_TESTS();

	std::cout << "Unit testing was finished" << std::endl;

	return return_value;
}
