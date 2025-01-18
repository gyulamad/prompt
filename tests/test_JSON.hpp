#pragma once

#include <cassert>

#include "../TEST.hpp"
#include "../JSON.hpp"

void test_JSON_is_valid_json() {
    assert(is_valid_json("{\"key\":\"value\"}"));
    assert(!is_valid_json("invalid_json"));
}

void test_JSON_get_json_error() {
    string invalid_json = "invalid_json";
    assert(!is_valid_json(invalid_json));
    assert(get_json_error(invalid_json) != "");
    assert(get_json_error("{\"key\":\"value\"}") == "");
}

void test_JSON_get_json_value_type() {
    string json = "{\"key1\": \"value\", \"key2\": 42, \"key3\": true, \"array\": [1, 2, 3]}";
    assert(get_json_value_type(json, "key1") == JSON_TYPE_STRING);
    assert(get_json_value_type(json, "key2") == JSON_TYPE_INTEGER);
    assert(get_json_value_type(json, "key3") == JSON_TYPE_BOOLEAN);
    assert(get_json_value_type(json, "array[0]") == JSON_TYPE_INTEGER);
    assert(get_json_value_type(json, "nonexistent") == JSON_TYPE_UNDEFINED);
}

void test_JSON_json_get_string() {
    string json = "{\"key\":\"value\"}";
    assert(json_get_string(json, ".key") == "value");
}

void test_JSON_json_get_int() {
    string json = "{\"key\": 42}";
    assert(json_get_int(json, ".key") == 42);
}

void test_JSON_json_get_double() {
    string json = "{\"key\": 3.14}";
    assert(json_get_double(json, ".key") == 3.14);
}

void test_JSON_json_get_bool() {
    string json = "{\"key\": true}";
    assert(json_get_bool(json, ".key") == true);
}

void test_JSON_json_get_array() {
    string json_str = R"({"tags": ["developer", "engineer"], "name": "Alice"})";

    // Positive Test: Valid Array
    assert(json_get_array(json_str, ".tags") == R"(["developer","engineer"])");
}

void test_JSON_json_get_object() {
    string json_str = R"({"address": {"city": "New York", "zip": "10001"}, "tags": ["developer", "engineer"]})";

    // Positive Test: Valid Object
    assert(json_get_object(json_str, ".address") == R"({"city":"New York","zip":"10001"})");
}

// Test for the _json_selector function
void test_JSON_json_selector() {
    // Test 1: Basic key selection
    string selector = ".key1";
    json::json_pointer result = _json_selector(selector);
    assert(result.to_string() == "/key1");
    
    // Test 2: Nested keys
    selector = ".key1.key2";
    result = _json_selector(selector);
    assert(result.to_string() == "/key1/key2");
    
    // Test 3: Array index
    selector = ".array[0]";
    result = _json_selector(selector);
    assert(result.to_string() == "/array/0");
    
    // Test 4: Array index with nested key
    selector = ".array[0].key1";
    result = _json_selector(selector);
    assert(result.to_string() == "/array/0/key1");
    
    // Test 5: Escaped dots within keys
    selector = ".key.with.dot";
    result = _json_selector(selector);
    assert(result.to_string() == "/key/with/dot");

    // Test 6: Array index with escaped dot
    selector = ".array[0].key.with.dot";
    result = _json_selector(selector);
    assert(result.to_string() == "/array/0/key/with/dot");

    bool throws;
}


void test_JSON_get() {
    // Test JSON string
    string jsonString = R"({"key1": "value", "key2": 42, "key3": true, "array": [1, 2, 3]})";
    
    // Create a JSON object from the string
    JSON jsonObj(jsonString);

    // Test getString() for existing key
    assert(jsonObj.get<string>(".key1") == "value");  // Expecting "value"

    // Test getInt() for existing key
    assert(jsonObj.get<int>(".key2") == 42);  // Expecting 42

    // Test getBool() for existing key
    assert(jsonObj.get<bool>(".key3") == true);  // Expecting true

    // Test getInt() for array element
    assert(jsonObj.get<int>(".array[0]") == 1);  // Expecting 1

    // Test isDefined() for existing key
    assert(jsonObj.isDefined(".key1") == true);  // Expecting true

    // Test isDefined() for non-existing key
    assert(jsonObj.isDefined(".nonexistent") == false);  // Expecting false

    // Test isNull() for an existing key with a non-null value
    assert(jsonObj.isNull(".key1") == false);  // Expecting false because key1 is not null

    // Test isNull() for a null key (this is a valid test if you add null value in the JSON)
    string nullJsonString = R"({"key1": null})"; // Adding null value for key1
    JSON nullJsonObj(nullJsonString);
    assert(nullJsonObj.isNull(".key1") == true);  // Expecting true because key1 is null
}

void test_JSON_set() {
    // Test JSON string
    string jsonString = R"({"key1": "value", "key2": 42, "key3": true, "array": [1, 2, 3]})";
    
    // Create a JSON object from the string
    JSON jsonObj(jsonString);

    // Test set() for existing key
    jsonObj.set(".key1", "new_value");  // Change value of key1 to "new_value"
    assert(jsonObj.get<string>(".key1") == "new_value");  // Expecting "new_value"

    // Test set() for integer value
    jsonObj.set(".key2", 100);  // Change value of key2 to 100
    assert(jsonObj.get<int>(".key2") == 100);  // Expecting 100

    // Test set() for boolean value
    jsonObj.set(".key3", false);  // Change value of key3 to false
    assert(jsonObj.get<bool>(".key3") == false);  // Expecting false

    // Test set() for array element
    jsonObj.set(".array[1]", 99);  // Change second element in array to 99
    assert(jsonObj.get<int>(".array[1]") == 99);  // Expecting 99

    // Test set() for a new key-value pair
    jsonObj.set(".newKey", "new_value_for_new_key");  // Add new key-value pair
    assert(jsonObj.get<string>(".newKey") == "new_value_for_new_key");  // Expecting "new_value_for_new_key"

    // Test set() for a new boolean key
    jsonObj.set(".newBoolKey", true);  // Add a new boolean key
    assert(jsonObj.get<bool>(".newBoolKey") == true);  // Expecting true

    // Test set() for a new null value (assuming your set method supports null)
    jsonObj.set(".newNullKey", nullptr);  // Add a new null value key
    assert(jsonObj.isNull(".newNullKey") == true);  // Expecting true because the value is null

    // TODO Test set() with nested object (creating a new object)
    // jsonObj.set(".nestedObject", R"({"nestedKey": "nestedValue"})");  // Add a new object
    // assert(jsonObj.get<string>(".nestedObject.nestedKey") == "nestedValue");  // Expecting "nestedValue"
}



void test_JSON() {
    TEST(test_JSON_json_selector);
    TEST(test_JSON_is_valid_json);
    TEST(test_JSON_get_json_error);
    TEST(test_JSON_get_json_value_type);
    TEST(test_JSON_json_get_string);
    TEST(test_JSON_json_get_int);
    TEST(test_JSON_json_get_double);
    TEST(test_JSON_json_get_bool);
    TEST(test_JSON_json_get_array);
    TEST(test_JSON_json_get_object);
    TEST(test_JSON_get);
    TEST(test_JSON_set);
}
