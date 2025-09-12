#pragma once
#include "defines.h"

//Returns lenght of the given string
FAPI u64 string_length(const char* str);
FAPI char* string_duplicate(const char* str);
//Case-sensitive string comparison. True if the same, otherwise false
FAPI b8 string_equal(const char* str0, const char* str1);