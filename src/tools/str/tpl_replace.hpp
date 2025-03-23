#pragma once

#include <string>
#include <map>

#include "../utils/ERROR.hpp"
#include "../regx/regx_match.hpp"
#include "../regx/regx_match_all.hpp"
#include "../containers/array_keys.hpp"
#include "../containers/in_array.hpp"
#include "../str/implode.hpp"

#include "str_cut_end.hpp"
#include "str_replace.hpp"
#include "str_contains.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::regx;
using namespace tools::containers;
using namespace tools::str;

namespace tools::str {

    string tpl_replace(const map<string, string>& replacements, const string& template_str, const string& placeholder_ptrn = "\\{\\{[^}]+\\}\\}") {
        
        // Check if all provided replacements exist in template        
        for (const auto& pair : replacements) {
            if (!regx_match("^" + placeholder_ptrn + "$", pair.first))
                throw ERROR(
                    "Replacement variable provided that does not match to the placeholder regex: " + pair.first + 
                    ", pattern: ^" + placeholder_ptrn + "$" + 
                    "\nTemplate: " + str_cut_end(template_str));
            if (!str_contains(template_str, pair.first))
                throw ERROR(
                    "Replacement variable provided for a template that is not having this placeholder: " + pair.first + 
                    "\nTemplate: " + str_cut_end(template_str));
        }
        
        // Check if all template expected variable are provided
        vector<string> matches;
        regx_match_all(placeholder_ptrn, template_str, &matches);
        vector<string> variables = array_keys(replacements);
        for (const string& match: matches)
            if (!in_array(match, variables))
                throw ERROR(
                    "Replacement value is not provided for the following placeholder(s): " + 
                    implode(", ", matches) + 
                    "\nTemplate: " + str_cut_end(template_str));

        // If validation passes, use the existing str_replace function
        return str_replace(replacements, template_str);
    }

    // Overload for single replacement
    string tpl_replace(const string& from, const string& to, const string& subject, const string& placeholder_ptrn = "\\{\\{[^}]+\\}\\}") {
        return tpl_replace({{from, to}}, subject, placeholder_ptrn);
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_tpl_replace_single_replacement() {
    string input = "Hello {{name}}!";
    string expected = "Hello John!";
    string actual = tpl_replace("{{name}}", "John", input);
    assert(actual == expected && "test_tpl_replace_single_replacement failed");
}

void test_tpl_replace_multiple_replacements() {
    string input = "{{greeting}}, {{name}}! Welcome to {{place}}.";
    map<string, string> replacements = {
        {"{{greeting}}", "Hi"},
        {"{{name}}", "Alice"},
        {"{{place}}", "Wonderland"}
    };
    string expected = "Hi, Alice! Welcome to Wonderland.";
    string actual = tpl_replace(replacements, input);
    assert(actual == expected && "test_tpl_replace_multiple_replacements failed");
}

void test_tpl_replace_no_placeholder_in_template() {
    bool thrown = false;
    try {
        string input = "Hello World!";
        tpl_replace("{{name}}", "John", input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_no_placeholder_in_template failed");
}

void test_tpl_replace_missing_replacement_value() {
    bool thrown = false;
    try {
        string input = "Hello {{name}} and {{friend}}!";
        map<string, string> replacements = {{"{{name}}", "John"}};
        tpl_replace(replacements, input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_missing_replacement_value failed");
}

void test_tpl_replace_invalid_placeholder_regex() {
    bool thrown = false;
    try {
        string input = "Hello [name]!";
        tpl_replace("[name]", "John", input, "\\{\\{[^}]+\\}\\}");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_invalid_placeholder_regex failed");
}

void test_tpl_replace_empty_template() {
    bool thrown = false;
    try {
        string input = "";
        string expected = "";
        string actual = tpl_replace("{{name}}", "John", input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_empty_template failed");
}

void test_tpl_replace_empty_replacement_value() {
    string input = "Hello {{name}}!";
    string expected = "Hello !";
    string actual = tpl_replace("{{name}}", "", input);
    assert(actual == expected && "test_tpl_replace_empty_replacement_value failed");
}

TEST(test_tpl_replace_single_replacement);
TEST(test_tpl_replace_multiple_replacements);
TEST(test_tpl_replace_no_placeholder_in_template);
TEST(test_tpl_replace_missing_replacement_value);
TEST(test_tpl_replace_invalid_placeholder_regex);
TEST(test_tpl_replace_empty_template);
TEST(test_tpl_replace_empty_replacement_value);
#endif
