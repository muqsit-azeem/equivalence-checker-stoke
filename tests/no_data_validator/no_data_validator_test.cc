//
// Created by andrea on 7/25/25.
//
#include <gtest/gtest.h>

#include "../../src/no_data_validator/regexOperations/plusOperation.h"

class RegExTestFixture : public ::testing::Test {
protected:
  stoke::PlusOperation;

  void SetUp() override {
    a = 2;
    b = 3;
  }

  void TearDown() override {
    // Optional cleanup
  }
};


TEST(RegEx, AddToOperation) {

}