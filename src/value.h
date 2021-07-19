#ifndef VALUE_H
#define VALUE_H

#include <s840d_def.h>

#include <string>
#include <variant>
#include <optional>

typedef int32_t       s840d_int_t;
typedef double        s840d_real_t;
typedef bool          s840d_bool_t;
typedef uint8_t       s840d_char_t;
typedef std::string   s840d_string_t;

// keep in sync with ValueType
typedef std::variant<s840d_int_t, s840d_real_t, s840d_bool_t, s840d_char_t, s840d_string_t> Value;

// keep in sync with Value i.e. std::variant::index()
enum class ValueType
{
    TYPE(DEF_TYPE_ENUM)
};

std::optional<ValueType> valueTypeFromString(const std::string& s);
Value createDefaultValue(ValueType type);
constexpr ValueType getValueType(const Value& value)
{
    return static_cast<ValueType>(value.index());
}


s840d_real_t   assignCastReal(const Value& v);
s840d_int_t    assignCastInt(const Value& v);
s840d_bool_t   assignCastBool(const Value& v);
s840d_char_t   assignCastChar(const Value& v);
s840d_string_t assignCastString(const Value& v);

/**
 * Implements S840D automatic type convertions on assignment.
 * See PGA, 2.8 Possible type conversions.
 * Throws Alarm 12150 when type conversion is not possible.
 */
Value assignCast(const Value& v, ValueType t);

template<class... Ts> struct make_visitor : Ts... { using Ts::operator()...; };
template<class... Ts> make_visitor(Ts...) -> make_visitor<Ts...>;



#endif // VALUE_H
