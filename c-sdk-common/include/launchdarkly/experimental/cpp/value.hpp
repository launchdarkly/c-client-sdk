#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <utility> // initializer_list, pair, swap
#include <cstddef> // size_t

extern "C" {
#include <launchdarkly/memory.h>
#include <launchdarkly/experimental/ldvalue.h>
}

namespace launchdarkly {

class Value;
class Array {
  public:
    ~Array();
    explicit Array();
    Array(std::initializer_list<Value>);
    Array(const Array&) = delete;
    Array(Array&&) = delete;
    Array& operator=(const Array&) = delete;
    Array& operator=(Array&&) noexcept = delete;
    friend bool operator== (const Array&, const Array&) = delete;

    Array& add(const Value&);
    Array& add(Value&&);
    Array& add(bool);
    Array& add(double);
    Array& add(const char *);
    Array& add(const std::string&);

    Value build() const;
  private:
    struct LDArray *ptr;
}; // class Array



class Object {
  public:
    ~Object();
    explicit Object();
    Object(std::initializer_list<std::pair<const char*, Value>>);
    Object(const Object&) = delete;
    Object(Object&&) = delete;
    Object& operator=(const Object&) = delete;
    Object& operator=(Object&&) noexcept = delete;
    friend bool operator== (const Object&, const Object&) = delete;

    void set(const char* key, const Value& value);
    void set(const char *key, Value&& value);

    void set(const char *key, bool value);
    void set(const char *key, double value);
    void set(const char *key, const char *value);
    void set(const char *key, const std::string& value);

    Value build() const;
  private:
    struct LDObject *ptr;
}; // class Object

class Value {
  public:
    enum class Type {
        /**
        * The value's type is unrecognized.
        */
        Unrecognized,
        /**
        * The value is null.
        */
        Null,
        /**
        * The value is a boolean.
        */
        Bool,
        /**
        * The value is a number. JSON does not have separate types for integers and floats.
        */
        Number,
        /**
        * The value is a string.
        */
        String,
        /**
        * The value is an array.
        */
        Array,
        /**
        * The value is an object.
        */
        Object
    };

    /**
     * Constructs a Value from a raw pointer.
     * Ownership is transferred to the Value.
     * @param ptr Raw pointer.
     */
    explicit Value(struct LDValue *ptr)
    :ptr{ptr} {}

    /**
     * Constructs a null-type Value. Represents
     * the JSON "null".
     */
    explicit Value()
    :ptr {LDValue_Null()} {}

    /**
     * Constructs a Value from a boolean.
     * @param b Boolean value.
     */
    explicit Value(bool b)
    :ptr{LDValue_Bool(b ? LDBooleanTrue : LDBooleanFalse)} {}

    /**
     * Constructs a Value from a double.
     * @param b Double value.
     */
    explicit Value(double b)
    :ptr { LDValue_Number(b) } {}

    /**
     * Constructs a Value from a string literal.
     * @param b String literal. The pointer must remain valid
     * for the duration of the program.
     */
    explicit Value(const char *b)
    :ptr { LDValue_ConstantString(b)} {}

    /**
     * Constructs a Value from an std::string.
     * @param b String object. The string is cloned.
     */
    explicit Value(const std::string& b)
    :ptr { LDValue_OwnedString(b.c_str())} {}

    /**
     * Implicit construction of a Value from an Array builder.
     * @param array Array builder.
     */
    Value(const Array& array)
    :ptr{ array.build().release() } {}

    /**
     * Implicit construction of a Value from an Object builder.
     * @param obj Object builder.
     */
    Value(const Object& obj)
    :ptr{ obj.build().release() } {}

    /**
     * Constructs a Value from another Value.
     * @param other The Value to clone.
     */
    Value(const Value& other)
    :ptr { LDValue_Clone(other.ptr) } {}

    /**
     * Move constructs a Value from another Value.
     * @param other The Value to consume.
     */
    Value(Value&& other) noexcept
    :ptr {other.release()} {}

    /**
     * Copy assigns this Value from another Value.
     * @param other Value to copy.
     */
    Value& operator=(const Value& other) {
        return *this = Value(other);
    }

    /**
     * Move assigns this Value from another Value.
     * @param other The Value to move from.
     */
    Value& operator=(Value&& other) noexcept
    {
        // destructor of other will free this object's previous ptr.
        std::swap(ptr, other.ptr);
        return *this;
    }

    /**
     * Destroys the Value and any child Values.
     */
    ~Value() {
        LDValue_Free(ptr);
    }


    friend bool operator== (const Value& c1, const Value& c2) {
        return LDValue_Equal(c1.ptr, c2.ptr);
    }

    friend bool operator!= (const Value& c1, const Value& c2) {
        return !(c1 == c2);
    }


    /**
     * Returns the Value's type.
     * @return Type of Value.
     */
    Type type() const {
        switch (LDValue_Type(ptr)) {
        case LDValueType_Unrecognized:
            return Type::Unrecognized;
        case LDValueType_Null:
            return Type::Null;
        case LDValueType_Bool:
            return Type::Bool;
        case LDValueType_Number:
            return Type::Number;
        case LDValueType_String:
            return Type::String;
        case LDValueType_Array:
            return Type::Array;
        case LDValueType_Object:
            return Type::Object;
        }
    }

    /**
     * Serializes the Value to a compact JSON string.
     * @return JSON string.
     */
    std::string serialize_json() {
        char *serialized = LDValue_SerializeJSON(ptr);
        std::string r = serialized;
        LDFree(serialized);
        return r;
    }

    /**
     * Serializes the Value to formatted JSON string.
     * @return JSON string.
     */
    std::string serialize_formatted_json() {
        char *serialized = LDValue_SerializeFormattedJSON(ptr);
        std::string r = serialized;
        LDFree(serialized);
        return r;
    }

    /**
     * Constructs a Value from a JSON string.
     * @param json Input JSON string.
     * @return Constructed Value. The Value may be of type null
     * if the JSON couldn't be parsed.
     */
    static Value parse_json(const char *json) {
        struct LDValue* parsed = LDValue_ParseJSON(json);
        if (!parsed) {
            return Value();
        }
        return Value(parsed);
    }

    /**
     * Retrieve the boolean value.
     * @return Boolean value, or false if not a bool.
     */
    bool get_bool() const {
        return LDValue_GetBool(ptr);
    }

    /**
     * Retrieve the string value.
     * @return String value, or empty string if not a string.
     */
    std::string get_string() const {
        return LDValue_GetString(ptr);
    }

    /**
     * Retrieve the number value.
     * @return Number value, or 0.0 if not a number.
     */
    double get_number() const {
        return LDValue_GetNumber(ptr);
    }

    /**
     * Construct a vector of individual Values from an array-type (or object-type)
     * Value. The new values are cloned from the Value, which may be an
     * expensive operation.
     * @return Vector of Values if array-type/object-type, or an empty vector.
     */
    std::vector<Value>
    into_vector() const {
        std::vector<Value> values;
        for (struct LDIter *it = LDValue_GetIter(ptr); it; it = LDIter_Next(it)) {
            values.emplace_back(LDValue_Clone(LDIter_Val(it)));
        }
        return values;
    }

    /**
     * Construct an unordered map of key/Values from an object-type Value.
     * The new keys/values are cloned from the Value, which may be an expensive
     * operation.
     * @return Map of key/values if object-type, otherwise empty unordered map.
     */
    std::unordered_map<std::string, Value>
    into_unordered_map() const {
        std::unordered_map<std::string, Value> map;
        if (this->type() != Type::Object) {
            return map;
        }
        for (struct LDIter *it = LDValue_GetIter(ptr); it; it = LDIter_Next(it)) {
            map.emplace(LDIter_Key(it), LDValue_Clone(LDIter_Val(it)));
        }
        return map;
    }

    /**
     * Count how many elements are in an array-type or object-type Value.
     * @return Element count, or 0 if not array or object-type.
     */
    size_t count() const {
        return LDValue_Count(ptr);
    }

    /**
     * Check if Value represents null.
     * @return True if null-type.
     */
    bool is_null() const {
        return type() == Type::Null;
    }

    /**
     * Releases the managed LDValue pointer.
     * Only necessary when interfacing with C code where ownership
     * must be transferred.
     * @return Previously managed pointer. The caller is now responsible
     * for managing its lifetime.
     */
    struct LDValue* release() {
        struct LDValue* raw = ptr;
        ptr = nullptr;
        return raw;
    }


  private:
    struct LDValue *ptr;
}; // class Value


Array::~Array() {
    LDArray_Free(ptr);
}

Array::Array() :ptr{LDArray_New()} {}

Array::Array(std::initializer_list<Value> init) :ptr{LDArray_New()} {
    for (auto copy : init) {
        LDArray_Add(ptr, copy.release());
    }
}

Array& Array::add(const Value& v) {
    Value clone {v};
    LDArray_Add(ptr, clone.release());
    return *this;
}

Array& Array::add(Value&& v) {
    LDArray_Add(ptr, v.release());
    return *this;
}

Array& Array::add(bool b) { add(Value(b)); return *this;}
Array& Array::add(double b) { add(Value(b)); return *this; }
Array& Array::add(const char *b) { add(Value(b)); return *this; }
Array& Array::add(const std::string& b) { add(Value(b)); return *this; }

Value Array::build() const {
    return Value(LDArray_Build(ptr));
}


Object::Object()
:ptr{LDObject_New()} {}

Object::Object(std::initializer_list<std::pair<const char*, Value>> init)
:ptr{LDObject_New()} {
    for (auto kv : init) {
        LDObject_AddConstantKey(ptr, kv.first, kv.second.release());
    }
}

Object::~Object() {
    LDObject_Free(ptr);
}


void Object::set(const char* key, const Value& value) {
    Value clone {value};
    LDObject_AddConstantKey(ptr, key, clone.release());
}

void Object::set(const char *key, Value&& value) {
    LDObject_AddConstantKey(ptr, key, value.release());
}

void Object::set(const char *key, bool value) { set(key, Value(value)); }
void Object::set(const char *key, double value) { set(key, Value(value)); }
void Object::set(const char *key, const char *val) { set(key, Value(val)); }
void Object::set(const char *key, const std::string& val) { set(key, Value(val)); }

Value Object::build() const {
    return Value(LDObject_Build(ptr));
}

} // namespace launchdarkly
