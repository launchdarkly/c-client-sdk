#include "gtest/gtest.h"
#include "commonfixture.h"

#include <launchdarkly/experimental/cpp/value.hpp>

extern "C" {
#include <launchdarkly/memory.h>
}

using namespace launchdarkly;

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class LDValueFixture : public CommonFixture {
};

// Requires valgrind leak checker
TEST_F(LDValueFixture, ObjectIsFreed) {
    Object obj;
}

// Requires valgrind leak checker
TEST_F(LDValueFixture, ArrayIsFreed) {
    Array array;
}

// Requires valgrind leak checker
TEST_F(LDValueFixture, ObjectToValueIsFreed) {
    Object obj;
    Value v = obj;
}

// Requires valgrind leak checker
TEST_F(LDValueFixture, ArrayToValueIsFreed) {
    Array array;
    Value v = array;
}

// Requires valgrind leak checker
TEST_F(LDValueFixture, PrimitivesAreFreed) {
     Value boolVal{true};
     Value constantStringVal{"hello"};
     Value ownedString{std::string{"hello"}};
     Value numVal(12.0);
     Value nullVal;
}

// Requires valgrind leak checker
TEST_F(LDValueFixture, ObjectOwnedKeyIsFreed) {
    struct LDObject *obj = LDObject_New();
    LDObject_AddOwnedKey(obj, "key", LDValue_True());

    struct LDValue *objVal = LDObject_Build(obj);
    LDValue_Free(objVal);

    LDObject_Free(obj);
}

TEST_F(LDValueFixture, NullPointerHasInvalidType) {
    ASSERT_EQ(LDValue_Type(nullptr), LDValueType_Unrecognized);
}

TEST_F(LDValueFixture, PrimitivesHaveCorrectType) {
    Value boolVal{true};
    Value constantStringVal{"hello"};
    Value ownedString{std::string{"hello"}};
    Value numVal{12.0};
    Value nullVal;

    ASSERT_EQ(boolVal.type(), Value::Type::Bool);
    ASSERT_EQ(constantStringVal.type(), Value::Type::String);
    ASSERT_EQ(ownedString.type(), Value::Type::String);
    ASSERT_EQ(numVal.type(), Value::Type::Number);
    ASSERT_EQ(nullVal.type(), Value::Type::Null);
}

TEST_F(LDValueFixture, CloneHasCorrectValue) {
    Value boolVal{true};
    Value constantStringVal{"hello"};
    Value ownedStringVal{std::string{"hello"}};
    Value numVal{12.0};
    Value nullVal;

    Value boolClone = boolVal;
    Value constantStringClone = constantStringVal;
    Value ownedStringClone = ownedStringVal;
    Value numClone = numVal;
    Value nullClone = nullVal;

    ASSERT_EQ(boolVal, boolClone);
    ASSERT_EQ(constantStringVal, constantStringClone);
    ASSERT_EQ(ownedStringVal, ownedStringClone);
    ASSERT_EQ(numVal, numClone);
    ASSERT_EQ(nullVal, nullClone);
}

TEST_F(LDValueFixture, ArrayHasCorrectType) {
    Value value{Array{}};
    ASSERT_EQ(value.type(), Value::Type::Array);

    Value clone = value;
    ASSERT_EQ(clone.type(), Value::Type::Array);
    ASSERT_EQ(value, clone);
}

TEST_F(LDValueFixture, ObjectHasCorrectType) {
    Value value {Object{}};
    ASSERT_EQ(value.type(), Value::Type::Object);

    Value clone = value;
    ASSERT_EQ(clone.type(), Value::Type::Object);
    ASSERT_EQ(clone, value);
}

TEST_F(LDValueFixture, StringEqualityBetweenConstantAndOwned) {
    Value a{"a"};
    Value b{std::string{"a"}};

    ASSERT_EQ(a, b);
}

TEST_F(LDValueFixture, CountOfPrimitivesIsZero) {
    Value boolVal{true};
    Value constantStringVal{"hello"};
    Value ownedStringVal{std::string{"hello"}};
    Value numVal{12.0};
    Value nullVal;

    ASSERT_EQ(boolVal.count(), 0);
    ASSERT_EQ(numVal.count(), 0);
    ASSERT_EQ(constantStringVal.count(), 0);
    ASSERT_EQ(ownedStringVal.count(), 0);
    ASSERT_EQ(nullVal.count(), 0);
}


TEST_F(LDValueFixture, ObjectIsDisplayed) {

    Object obj = {
        {"bool", Value{true}},
        {"string", Value{"hello"}},
        {"array", Array{ Value{"foo"}, Value{"bar"}}}
    };

    std::string json = obj.build().serialize_json();

    ASSERT_EQ(json, "{\"bool\":true,\"string\":\"hello\",\"array\":[\"foo\",\"bar\"]}");

}

TEST_F(LDValueFixture, ObjectIsBuiltManyTimes) {
    Object obj;
    obj.set("key", "value");

    for (size_t i = 0; i < 10; i++) {
        std::string json = obj.build().serialize_json();
        ASSERT_EQ(json, "{\"key\":\"value\"}");
    }
}

TEST_F(LDValueFixture, ObjectCanAddValueAfterBuild) {
    Object obj;

    obj.set("key1", "value1");
    ASSERT_EQ(obj.build().count(), 1);

    obj.set("key2", "value2");
    ASSERT_EQ(obj.build().count(), 2);
}

TEST_F(LDValueFixture, ArrayIsBuiltManyTimes) {
    Array array{Value("value")};

    for (size_t i = 0; i < 10; i++) {
        std::string json = array.build().serialize_json();
        ASSERT_EQ(json, "[\"value\"]");
    }
}


TEST_F(LDValueFixture, ArrayCanAddValueAfterBuild) {
    Array array{Value("value1")};
    ASSERT_EQ(array.build().count(), 1);

    array.add(Value("value2"));
    ASSERT_EQ(array.build().count(), 2);
}

TEST_F(LDValueFixture, DisplayUser) {

    Object attrs {
       {"key", Value{"foo"}},
       {"name", Value{"bar"}},
       {"custom", Object{
           {"things", Array{Value{"a"}, Value{"b"}, Value{true}}}
        }}
    };

    std::string json = attrs.build().serialize_formatted_json();

    ASSERT_EQ(json, "{\n\t\"key\":\t\"foo\",\n\t\"name\":\t\"bar\",\n\t\"custom\":\t{\n\t\t\"things\":\t[\"a\", \"b\", true]\n\t}\n}");
}


TEST_F(LDValueFixture, IteratePrimitive) {
    Value v{false};

    ASSERT_TRUE(v.into_vector().empty());
    ASSERT_TRUE(v.into_unordered_map().empty());
}

TEST_F(LDValueFixture, GetBool) {
    Value trueVal{true};
    ASSERT_TRUE(trueVal.get_bool());

    Value falseVal{false};
    ASSERT_FALSE(falseVal.get_bool());
}

TEST_F(LDValueFixture, GetNumber) {
    Value value{12.0};
    ASSERT_EQ(12.0, value.get_number());
}

TEST_F(LDValueFixture, GetString) {
    Value value {std::string("hello")};
    ASSERT_EQ(value.get_string(), "hello");
}

TEST_F(LDValueFixture, IsNull) {
    Value value;
    ASSERT_EQ(value.type(), Value::Type::Null);
    ASSERT_TRUE(value.is_null());
}

TEST_F(LDValueFixture, ParseBool) {
    Value falseVal = Value::parse_json("false");
    ASSERT_EQ(falseVal.type(), Value::Type::Bool);
    ASSERT_FALSE(falseVal.get_bool());

    Value trueVal = Value::parse_json("true");
    ASSERT_EQ(trueVal.type(), Value::Type::Bool);
    ASSERT_TRUE(trueVal.get_bool());
}

TEST_F(LDValueFixture, ParseNumber) {
    Value numVal = Value::parse_json("12.34567");
    ASSERT_EQ(12.34567, numVal.get_number());
}

TEST_F(LDValueFixture, ParseNull) {
    Value nullVal = Value::parse_json("null");
    ASSERT_TRUE(nullVal.is_null());
}

TEST_F(LDValueFixture, ParseString) {
    Value stringVal = Value::parse_json("\"hello world\"");
    ASSERT_EQ(stringVal.get_string(), "hello world");
}

TEST_F(LDValueFixture, ParseArray) {
    Array array{Value{true}, Value{"hello"}, Value{3.0}};

    Value b = Value::parse_json("[true, \"hello\", 3]");

    ASSERT_EQ(b, array);
}


TEST_F(LDValueFixture, ParseObject) {

    Object obj;
    obj.set("bool", true);
    obj.set("number", 12.34);
    obj.set("string", "hello");

    Value a = obj.build();

    Value b = Value::parse_json(R"({"bool": true, "number": 12.34, "string": "hello"})");

    ASSERT_EQ(a, b);
}

TEST_F(LDValueFixture, ParseObjectDuplicateKeysHasCorrectCount) {
    Value a = Value::parse_json(
        R"({"a": true, "a": 12.34})");

    ASSERT_EQ(a.count(), 2);
}

TEST_F(LDValueFixture, ParseObjectDuplicateKeysNotEqual) {
    Value a = Value::parse_json(R"({"a": true, "a": 12.34})");
    Value b = Value::parse_json(R"({"a": true, "a": 12.34})");

    ASSERT_EQ(a.count(), b.count());
    ASSERT_NE(a, b);
}


TEST_F(LDValueFixture, IterateHomogeneousArray) {
    Value array = Value::parse_json("[true, true, true, true]");
    for (const auto& item : array.into_vector()) {
        ASSERT_EQ(item.type(), Value::Type::Bool);
        ASSERT_TRUE(item.get_bool());
    }
}

TEST_F(LDValueFixture, IterateHeterogeneousArray) {
    Value b = Value::parse_json("[true, \"hello\", 3]");

    auto array = b.into_vector();
    ASSERT_EQ(array.size(), 3);
    ASSERT_EQ(array[0].type(), Value::Type::Bool);
    ASSERT_TRUE(array[0].get_bool());

    ASSERT_EQ(array[1].type(), Value::Type::String);
    ASSERT_EQ(array[1].get_string(), "hello");

    ASSERT_EQ(array[2].type(), Value::Type::Number);
    ASSERT_EQ(array[2].get_number(), 3);
}


TEST_F(LDValueFixture, IterateParsedObject) {
    Value obj = Value::parse_json(R"({"a": true, "b": false})");
    auto map = obj.into_unordered_map();

    ASSERT_EQ(map.at("a").type(), Value::Type::Bool);
    ASSERT_TRUE(map.at("a").get_bool());

    ASSERT_EQ(map.at("b").type(), Value::Type::Bool);
    ASSERT_FALSE(map.at("b").get_bool());
}

TEST_F(LDValueFixture, EqualArraysWithElements) {
    Value a = Value::parse_json(R"(["a", "b", "c"])");
    Value b = Value::parse_json(R"(["a", "b", "c"])");
    ASSERT_EQ(a, b);
}

TEST_F(LDValueFixture, EqualArraysWithoutElements) {
    Value a = Value::parse_json("[]");
    Value b = Value::parse_json("[]");
    ASSERT_EQ(a, b);
}


TEST_F(LDValueFixture, UnequalArrayElement) {
    Value a = Value::parse_json(R"(["a"])");
    Value b = Value::parse_json(R"(["b"])");
    ASSERT_NE(a, b);
}

TEST_F(LDValueFixture, UnequalArrayOrder) {
    Value a = Value::parse_json(R"(["a", "b"])");
    Value b = Value::parse_json(R"(["b", "a"])");
    ASSERT_NE(a, b);
}

TEST_F(LDValueFixture, EqualObjectWithElements) {
    Value a = Value::parse_json(R"({"a" : true, "b" : false})");
    Value b = Value::parse_json(R"({"a" : true, "b" : false})");
    ASSERT_EQ(a, b);
}

TEST_F(LDValueFixture, EqualObjectWithElementsOutOfOrder) {
    Value a = Value::parse_json(R"({"a" : true, "b" : false})");
    Value b = Value::parse_json(R"({"b" : false, "a" : true})");
    ASSERT_EQ(a, b);
}

TEST_F(LDValueFixture, EqualObjectWithoutElements) {
    Value a = Value::parse_json(R"({})");
    Value b = Value::parse_json(R"({})");
    ASSERT_EQ(a, b);
}


TEST_F(LDValueFixture, UnequalObjectKeys) {
    Value a = Value::parse_json(R"({"a" : true})");
    Value b = Value::parse_json(R"({"b" : true})");
    ASSERT_NE(a, b);
}


TEST_F(LDValueFixture, UnequalObjectValues) {
    Value a = Value::parse_json(R"({"a" : true})");
    Value b = Value::parse_json(R"({"a" : false})");
    ASSERT_NE(a, b);
}


TEST_F(LDValueFixture, UnequalObjectSize) {
    Value a = Value::parse_json(R"({"a" : true})");
    Value b = Value::parse_json(R"({"a" : true, "b" : true})");
    ASSERT_NE(a, b);
}


