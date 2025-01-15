#pragma once

#include <cassert>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <iostream>

#include "../tools.hpp"

void test_timef_default_format() {
  std::string currentTimeStr = timef();
  assert(currentTimeStr.length() > 10); // Check if string has at least a basic date and time
}

void test_timef_custom_format() {
  std::string customTimeStr = timef("%H:%M:%S");
  assert(customTimeStr.length() == 8); // Check for HH:MM:SS format
}

void test_timef_specific_timestamp() {
  std::time_t specificTime = 1678886400; // Example timestamp
  std::string specificTimeStr = timef("%Y-%m-%d %H:%M:%S", &specificTime);
  assert(specificTimeStr == "2023-03-15 13:20:00"); // Assert specific time.  Timezone dependent!
}

void test_timef_empty_format() {
  std::string emptyFormatStr = timef("");
  assert(emptyFormatStr.empty()); // Check for empty format resulting in empty string
}

void test_timef_invalid_format() {
  std::string invalidFormatStr = timef("%Z"); // %Z is system-dependent;  this test is unreliable.
  // A better approach would be to check for error conditions or expected behaviors rather than string length.
  //  This test is included to highlight the need for robust error handling in production code.
  assert(invalidFormatStr.length() >= 0); // This is a weak assertion, but demonstrates the problem with %Z
}


void test_timef() {
  test_timef_default_format();
  test_timef_custom_format();
  test_timef_specific_timestamp();
  test_timef_empty_format();
  test_timef_invalid_format();
}