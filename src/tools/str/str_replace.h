#include <string>
#include <map>

#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace tools::str {
    string str_replace(const map<string, string>& v, const string& s);
    string str_replace(const string& from, const string& to, const string& subject);
}


#ifdef TEST

void test_str_replace_single_replacement();
void test_str_replace_multiple_occurrences();
void test_str_replace_no_match();
void test_str_replace_empty_to_multiple_occurrences();
void test_str_replace_empty_to_no_match();
void test_str_replace_non_empty_to();
void test_str_replace_empty_input();
void test_str_replace_empty_to();
void test_str_replace_empty_from();
void test_str_replace_map_multiple_replacements();
void test_str_replace_map_overlapping_keys();
void test_str_replace_map_empty_input();

#endif