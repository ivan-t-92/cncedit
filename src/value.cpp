#include "value.h"
#include "s840d_alarm.h"
#include "s840d_def.h"

#include <cmath>
#include <stdexcept>
#include <array>
#include <algorithm>

static auto& valueTypeStrings()
{
    static std::array strings {TYPE(DEF_TYPE_STRING)};
    return strings;
}

std::optional<ValueType> valueTypeFromString(const std::string& s)
{
    auto& strings {valueTypeStrings()};
    auto it = std::find(strings.begin(), strings.end(), s);
    if (it != strings.end())
        return static_cast<ValueType>(it - strings.begin());

    return std::nullopt;
}

Value createDefaultValue(ValueType type)
{
    Value v;
    switch (type)
    {
    case ValueType::INT:
        v = 0;
        break;
    case ValueType::REAL:
        v = 0.0;
        break;
    case ValueType::BOOL:
        v = false;
        break;
    case ValueType::CHAR:
        v = (char)0;
        break;
    case ValueType::STRING:
        v = s840d_string_t{};
        break;
    }
    return v;
}

s840d_real_t assignCastReal(const Value& v)
{
    switch (getValueType(v))
    {
    case ValueType::INT:
        return std::get<s840d_int_t>(v);
    case ValueType::REAL:
        return std::get<s840d_real_t>(v);
    case ValueType::BOOL:
        return std::get<s840d_bool_t>(v) ? 1.0 : 0.0;
    case ValueType::CHAR:
        return std::get<s840d_char_t>(v);
    default:
        throw S840D_Alarm{12150};
    }
}

s840d_int_t assignCastInt(const Value& v)
{
    switch (getValueType(v))
    {
    case ValueType::INT:
        return std::get<s840d_int_t>(v);
    case ValueType::REAL:
    {
        long l = std::lround(std::get<s840d_real_t>(v));
        if (l > std::numeric_limits<s840d_int_t>::max() ||
            l < std::numeric_limits<s840d_int_t>::min())
        {
            throw S840D_Alarm{12150};
        }
        return static_cast<s840d_int_t>(l);
    }
    case ValueType::BOOL:
        return std::get<s840d_bool_t>(v) ? 1 : 0;
    case ValueType::CHAR:
        return std::get<s840d_char_t>(v);
    default:
        throw S840D_Alarm{12150};
    }
}

s840d_bool_t assignCastBool(const Value& v)
{
    switch (getValueType(v))
    {
    case ValueType::INT:
        return std::get<s840d_int_t>(v) != 0;
    case ValueType::REAL:
        return std::abs(std::get<s840d_real_t>(v)) != 0.0;
    case ValueType::BOOL:
        return std::get<s840d_bool_t>(v);
    case ValueType::CHAR:
        return std::get<s840d_char_t>(v) != 0;
    case ValueType::STRING:
        return !std::get<s840d_string_t>(v).empty();
    default:
        throw S840D_Alarm{12150};
    }
}

s840d_char_t assignCastChar(const Value& v)
{
    switch (getValueType(v))
    {
    case ValueType::INT:
    {
        s840d_int_t i = std::get<s840d_int_t>(v);
        if (i > std::numeric_limits<s840d_char_t>::max() ||
            i < std::numeric_limits<s840d_char_t>::min())
        {
            throw S840D_Alarm{12150};
        }
        return static_cast<s840d_char_t>(i);
    }
    case ValueType::REAL:
    {
        long i = std::lround(std::get<s840d_int_t>(v));
        if (i > std::numeric_limits<s840d_char_t>::max() ||
            i < std::numeric_limits<s840d_char_t>::min())
        {
            throw S840D_Alarm{12150};
        }
        return static_cast<s840d_char_t>(i);
    }
    case ValueType::BOOL:
        return std::get<s840d_bool_t>(v);
    case ValueType::CHAR:
        return std::get<s840d_char_t>(v);
    case ValueType::STRING:
    {
        auto& s = std::get<s840d_string_t>(v);
        if (s.size() == 1)
        {
            return s[0];
        }
        throw S840D_Alarm{12150};
    }
    default:
        throw S840D_Alarm{12150};
    }
}

s840d_string_t assignCastString(const Value& v)
{
    switch (getValueType(v))
    {
    case ValueType::BOOL:
        return std::to_string(std::get<s840d_bool_t>(v));
    case ValueType::CHAR:
        return s840d_string_t{1, static_cast<char>(std::get<s840d_char_t>(v))};
    case ValueType::STRING:
        return std::get<s840d_string_t>(v);
    default:
        throw S840D_Alarm{12150};
    }
}

Value assignCast(const Value& v, ValueType t)
{
    switch (t)
    {
    case ValueType::INT:    return assignCastInt(v);
    case ValueType::REAL:   return assignCastReal(v);
    case ValueType::BOOL:   return assignCastBool(v);
    case ValueType::CHAR:   return assignCastChar(v);
    case ValueType::STRING: return assignCastString(v);
    }
    throw std::runtime_error{"unreachalble"};
}
