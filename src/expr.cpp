#include "util.h"
#include "expr.h"
#include "variables.h"
#include "s840d_alarm.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <optional>

BinaryOpExpr::BinaryOpExpr(std::unique_ptr<Expr> lhs,
                           std::unique_ptr<Expr> rhs,
                           BinaryOp op)
    : m_lhs(std::move(lhs)),
      m_rhs(std::move(rhs)),
      m_op(op)
{}

static std::optional<s840d_real_t> convertToReal(const Value& value)
{
    switch (getValueType(value))
    {
    case ValueType::INT:  return std::get<s840d_int_t>(value);
    case ValueType::REAL: return std::get<s840d_real_t>(value);
    case ValueType::BOOL: return std::get<s840d_bool_t>(value);
    case ValueType::CHAR: return std::get<s840d_char_t>(value);
    default:              return std::nullopt;
    }
}

static std::optional<s840d_bool_t> convertToBool(const Value& value)
{
    switch (getValueType(value))
    {
    case ValueType::INT:  return std::get<s840d_int_t>(value) != 0;
    case ValueType::REAL: return std::abs(std::get<s840d_real_t>(value)) != 0.0;
    case ValueType::BOOL: return std::get<s840d_bool_t>(value);
    case ValueType::CHAR: return std::get<s840d_char_t>(value) != 0;
    default:              return std::nullopt;
    }
}

static std::optional<s840d_int_t> castToInt(const Value& value)
{
    switch (getValueType(value))
    {
    case ValueType::CHAR: return std::get<s840d_char_t>(value);
    case ValueType::INT:  return std::get<s840d_int_t>(value);
    default:              return std::nullopt;
    }
}

struct AddImpl
{
    static s840d_char_t opChar(s840d_char_t x, s840d_char_t y)
    {
        return static_cast<s840d_char_t>(x + y);
    }
    static bool opInt(s840d_int_t x, s840d_int_t y, s840d_int_t* res)
    {
#ifdef __GNUC__
        return __builtin_sadd_overflow(x, y, res);
#else
        *res = x + y;
        int64_t res64 = (int64_t)x + (int64_t)y;
        return res64 != (int64_t)*res;
#endif
    }
    static bool opReal(s840d_real_t x, s840d_real_t y, s840d_real_t* res)
    {
        *res = x + y;
        return std::abs(*res) == std::numeric_limits<s840d_real_t>::infinity();
    }
};

struct SubImpl
{
    static s840d_char_t opChar(s840d_char_t x, s840d_char_t y)
    {
        return static_cast<s840d_char_t>(x - y);
    }
    static bool opInt(s840d_int_t x, s840d_int_t y, s840d_int_t* res)
    {
#ifdef __GNUC__
        return __builtin_ssub_overflow(x, y, res);
#else
        *res = x - y;
        int64_t res64 = (int64_t)x - (int64_t)y;
        return res64 != (int64_t)*res;
#endif
    }
    static bool opReal(s840d_real_t x, s840d_real_t y, s840d_real_t* res)
    {
        *res = x - y;
        return std::abs(*res) == std::numeric_limits<s840d_real_t>::infinity();
    }
};

struct MulImpl
{
    static s840d_char_t opChar(s840d_char_t x, s840d_char_t y)
    {
        return static_cast<s840d_char_t>(x * y);
    }
    static bool opInt(s840d_int_t x, s840d_int_t y, s840d_int_t* res)
    {
#ifdef __GNUC__
        return __builtin_smul_overflow(x, y, res);
#else
        *res = x * y;
        return x != (*res / y);
#endif
    }
    static bool opReal(s840d_real_t x, s840d_real_t y, s840d_real_t* res)
    {
        *res = x * y;
        return std::abs(*res) == std::numeric_limits<s840d_real_t>::infinity();
    }
};

template<typename Impl>
struct OverflowCheckOp
{
    static s840d_char_t opChar(s840d_char_t x, s840d_char_t y)
    {
        // no overflow check for char
        return Impl::opChar(x, y);
    }
    static s840d_int_t opInt(s840d_int_t x, s840d_int_t y)
    {
        s840d_int_t result;
        bool overflow = Impl::opInt(x, y, &result);
        if (!overflow)
            return result;

        throw S840D_Alarm{14051};
    }
    static s840d_real_t opReal(s840d_real_t x, s840d_real_t y)
    {
        s840d_real_t result;
        bool overflow = Impl::opReal(x, y, &result);
        if (!overflow)
            return result;

        throw S840D_Alarm{14051};
    }
};

template<typename DivImpl>
struct OverflowCheckDiv
{
    static s840d_real_t opChar(s840d_char_t x, s840d_char_t y)
    {
        if (y == 0)
            throw S840D_Alarm{14051};

        return op(x, y);
    }
    static s840d_real_t opInt(s840d_int_t x, s840d_int_t y)
    {
        if (y == 0)
            throw S840D_Alarm{14051};

        return op(x, y);
    }
    static s840d_real_t opReal(s840d_real_t x, s840d_real_t y)
    {
        return op(x, y);
    }
private:
    template<typename T>
    static s840d_real_t op(T x, T y)
    {
        s840d_real_t result;
        bool error = div(x, y, &result);
        if (!error)
            return result;

        throw S840D_Alarm{14051};
    }
    static bool div(s840d_real_t x, s840d_real_t y, s840d_real_t* res)
    {
        *res = DivImpl::div(x, y);
        return std::abs(*res) == std::numeric_limits<s840d_real_t>::infinity() ||
               std::isnan(*res);
    }
};

struct Div
{
    static s840d_real_t div(s840d_real_t x, s840d_real_t y)
    {
        return x / y;
    }
};

struct Mod
{
    static s840d_real_t div(s840d_real_t x, s840d_real_t y)
    {
        return fmod(x, y);
    }
};

template<typename Impl>
Value binaryArithmetic(const Value& v1, const Value& v2)
{
    auto charVal1 = std::get_if<s840d_char_t>(&v1);
    auto charVal2 = std::get_if<s840d_char_t>(&v2);
    if (charVal1 && charVal2)
        return Impl::opChar(*charVal1, *charVal2);

    if (auto intVal1 = castToInt(v1))
    {
        if (auto intVal2 = castToInt(v2))
            return Impl::opInt(*intVal1, *intVal2);

        if (auto doubleVal2 = std::get_if<s840d_real_t>(&v2))
            return Impl::opReal(*intVal1, *doubleVal2);

        throw S840D_Alarm{12150};
    }
    if (auto doubleVal1 = std::get_if<s840d_real_t>(&v1))
    {
        if (auto intVal2 = std::get_if<s840d_int_t>(&v2))
            return Impl::opReal(*doubleVal1, *intVal2);

        if (auto doubleVal2 = std::get_if<s840d_real_t>(&v2))
            return Impl::opReal(*doubleVal1, *doubleVal2);

        throw S840D_Alarm{12150};
    }
    throw S840D_Alarm{12150};
}


static Value negate(const Value& value)
{
    switch (getValueType(value))
    {
    case ValueType::INT:  return -std::get<s840d_int_t>(value); // s840d doesn't check against std::numeric_limits<s840d_int_t>::min()
    case ValueType::REAL: return -std::get<s840d_real_t>(value);
    default:              throw S840D_Alarm{12150};
    }
}

static s840d_bool_t logicalNot(const Value& value)
{
    if (auto b = convertToBool(value))
        return !*b;

    throw S840D_Alarm{12150};
}

static Value bitwiseNot(const Value& value)
{
    if (auto charVal = std::get_if<s840d_char_t>(&value))
        return static_cast<s840d_char_t>(~*charVal);

    if (auto intVal = std::get_if<s840d_int_t>(&value))
        return ~*intVal;

    throw S840D_Alarm{12150};
}

template<typename Impl>
s840d_bool_t binaryLogic(const Value& v1, const Value& v2)
{
    auto b1 = convertToBool(v1);
    auto b2 = convertToBool(v2);
    if (b1 && b2)
        return Impl::logicOp(*b1, *b2);

    throw S840D_Alarm{12150};
}

struct AndImpl
{
    static bool logicOp(bool x, bool y) { return x && y; }
};

struct OrImpl
{
    static bool logicOp(bool x, bool y) { return x || y; }
};

struct XorImpl
{
    static bool logicOp(bool x, bool y) { return x ^ y; }
};

template<typename Impl>
s840d_bool_t binaryCompare(const Value& v1, const Value& v2)
{
    auto convertToInt = [](const Value& value) -> std::optional<s840d_int_t>
    {
        switch (getValueType(value))
        {
        case ValueType::INT:  return std::get<s840d_int_t>(value);
        case ValueType::BOOL: return std::get<s840d_bool_t>(value);
        case ValueType::CHAR: return std::get<s840d_char_t>(value);
        default:              return std::nullopt;
        }
    };
    if (auto doubleVal1 = std::get_if<s840d_real_t>(&v1))
    {
        if (auto doubleVal2 = convertToReal(v2))
            return Impl::compareEps(*doubleVal1, *doubleVal2);

        throw S840D_Alarm{12150};
    }
    if (auto doubleVal2 = std::get_if<s840d_real_t>(&v2))
    {
        if (auto doubleVal1 = convertToReal(v1))
            return Impl::compareEps(*doubleVal1, *doubleVal2);

        throw S840D_Alarm{12150};
    }
    if (auto intVal1 = convertToInt(v1))
    {
        if (auto intVal2 = convertToInt(v2))
            return Impl::compare(*intVal1, *intVal2);

        throw S840D_Alarm{12150};
    }
    if (auto strVal1 = std::get_if<s840d_string_t>(&v1))
    {
        if (auto strVal2 = std::get_if<s840d_string_t>(&v2))
            return Impl::compare(*strVal1, *strVal2);
    }
    throw S840D_Alarm{12150};
}

static constexpr double S840D_EPSILON {4e-12};

struct Equals
{
    template<typename T>
    static s840d_bool_t compare(T x, T y)
    {
        return x == y;
    }
    static s840d_bool_t compareEps(s840d_real_t x, s840d_real_t y)
    {
        return std::abs(x - y) <= std::max(std::abs(x), std::abs(y)) * S840D_EPSILON;
    }
};

struct Less
{
    template<typename T>
    static s840d_bool_t compare(T x, T y)
    {
        return x < y;
    }
    static s840d_bool_t compareEps(s840d_real_t x, s840d_real_t y)
    {
        return x < (y - std::max(std::abs(x), std::abs(y)) * S840D_EPSILON);
    }
};

struct Greater
{
    template<typename T>
    static s840d_bool_t compare(T x, T y)
    {
        return x > y;
    }
    static s840d_bool_t compareEps(s840d_real_t x, s840d_real_t y)
    {
        return x > (y + std::max(std::abs(x), std::abs(y)) * S840D_EPSILON);
    }
};

template<typename Impl>
Value binaryBitwise(const Value& v1, const Value& v2)
{
    if (auto charVal1 = std::get_if<s840d_char_t>(&v1))
    {
        if (auto charVal2 = std::get_if<s840d_char_t>(&v2))
            return Impl::bitwiseOp(*charVal1, *charVal2);

        if (auto intVal2 = std::get_if<s840d_int_t>(&v2))
            return Impl::bitwiseOp(static_cast<s840d_int_t>(*charVal1), *intVal2);

        throw S840D_Alarm{12150};
    }
    if (auto intVal1 = std::get_if<s840d_int_t>(&v1))
    {
        if (auto charVal2 = std::get_if<s840d_char_t>(&v2))
            return Impl::bitwiseOp(*intVal1, static_cast<s840d_int_t>(*charVal2));

        if (auto intVal2 = std::get_if<s840d_int_t>(&v2))
            return Impl::bitwiseOp(*intVal1, *intVal2);

        throw S840D_Alarm{12150};
    }
    throw S840D_Alarm{12150};
}

struct BitwiseAndImpl
{
    template<typename T>
    static T bitwiseOp(T x, T y)
    {
        return x & y;
    }
};

struct BitwiseOrImpl
{
    template<typename T>
    static T bitwiseOp(T x, T y)
    {
        return x | y;
    }
};

struct BitwiseXorImpl
{
    template<typename T>
    static T bitwiseOp(T x, T y)
    {
        return x ^ y;
    }
};

Value BinaryOpExpr::evaluate(Variables& variables) const
{
    return evaluate(m_lhs.get(), m_rhs.get(), m_op, variables);
}

Value BinaryOpExpr::evaluate(Expr* lhs, Expr* rhs, BinaryOpExpr::BinaryOp op, Variables& variables)
{
    switch (op)
    {
    // Arithmetic
    case ADD:
        return binaryArithmetic<OverflowCheckOp<AddImpl>>(lhs->evaluate(variables), rhs->evaluate(variables));
    case SUB:
        return binaryArithmetic<OverflowCheckOp<SubImpl>>(lhs->evaluate(variables), rhs->evaluate(variables));
    case MUL:
        return binaryArithmetic<OverflowCheckOp<MulImpl>>(lhs->evaluate(variables), rhs->evaluate(variables));
    case DIV_FP:
        return binaryArithmetic<OverflowCheckDiv<Div>>   (lhs->evaluate(variables), rhs->evaluate(variables));
    case DIV_INT:
        return std::trunc(std::get<s840d_real_t>(
            binaryArithmetic<OverflowCheckDiv<Div>>   (lhs->evaluate(variables), rhs->evaluate(variables))));
    case MOD:
        return binaryArithmetic<OverflowCheckDiv<Mod>>   (lhs->evaluate(variables), rhs->evaluate(variables));

    // Logic
    case AND:
        return binaryLogic<AndImpl>(lhs->evaluate(variables), rhs->evaluate(variables));
    case OR:
        return binaryLogic<OrImpl> (lhs->evaluate(variables), rhs->evaluate(variables));
    case XOR:
        return binaryLogic<XorImpl>(lhs->evaluate(variables), rhs->evaluate(variables));

    // Comparison
    case EQUAL:
        return binaryCompare<Equals>(lhs->evaluate(variables), rhs->evaluate(variables));
    case NOTEQUAL:
        return !binaryCompare<Equals>(lhs->evaluate(variables), rhs->evaluate(variables));
    case GREATER:
        return binaryCompare<Greater>(lhs->evaluate(variables), rhs->evaluate(variables));
    case LESS:
        return binaryCompare<Less>(lhs->evaluate(variables), rhs->evaluate(variables));
    case GREATER_OR_EQUAL:
        return !binaryCompare<Less>(lhs->evaluate(variables), rhs->evaluate(variables));
    case LESS_OR_EQUAL:
        return !binaryCompare<Greater>(lhs->evaluate(variables), rhs->evaluate(variables));

    // Bitwise
    case BITWISE_AND:
        return binaryBitwise<BitwiseAndImpl>(lhs->evaluate(variables), rhs->evaluate(variables));
    case BITWISE_OR:
        return binaryBitwise<BitwiseOrImpl>(lhs->evaluate(variables), rhs->evaluate(variables));
    case BITWISE_XOR:
        return binaryBitwise<BitwiseXorImpl>(lhs->evaluate(variables), rhs->evaluate(variables));
    }
    throw std::runtime_error{"unreachable"};
}

LiteralExpr::LiteralExpr(Value value)
    : m_value(std::move(value))
{}

Value LiteralExpr::evaluate(Variables& /*variables*/) const
{
    return m_value;
}

VariableExpr::VariableExpr(std::string  varName)
    : m_varName(std::move(varName))
{}

Value VariableExpr::evaluate(Variables& variables) const
{
    auto [value, accessResult] = variables.getValue(m_varName);
    using AccessResult = Variables::AccessResult;
    switch (accessResult)
    {
    case AccessResult::Success:
        return value;
    case AccessResult::DoNotExists:
        throw S840D_Alarm{12550}; // name is not known or not defined
    case AccessResult::ArrayIndexOutOfBounds:
    case AccessResult::InvalidDimensionCount:
    case AccessResult::DimensionMismatch:
    case AccessResult::TypeMismatch:
        throw std::exception{}; // TODO
    }
    throw std::runtime_error{"unreachable"};
}

void VariableExpr::setValue(const Value& value, Variables& variables) const
{
    auto [oldvalue, accessResult] = variables.getValue(m_varName);
    if (accessResult != Variables::AccessResult::Success)
        throw S840D_Alarm{12550};

    variables.setValue(m_varName, assignCast(value, getValueType(oldvalue)));
}

void VariableExpr::setValues(const ArrayInitializer& values, Variables& variables) const
{
    if (values.numberOfElements() == 1)
        setValue(values.getAndEvaluate(0, variables), variables);
    else
        throw S840D_Alarm{14130}; // too many initialization values given
}

ArrayExpr::ArrayExpr(std::string varName, std::vector<Expr*> indicies)
    : m_varName(std::move(varName)),
      m_indicies(std::move(indicies))
{}

ArrayExpr::~ArrayExpr()
{
    for (auto expr : m_indicies)
        delete expr;
}

Value ArrayExpr::evaluate(Variables& variables) const
{
    std::array<s840d_int_t, 3> indicies {evaluateIndicies(variables)};
    auto [value, accessResult] = variables.getArrayValue(m_varName, indicies.begin(), indicies.begin() + m_indicies.size());
    using AccessResult = Variables::AccessResult;
    switch (accessResult)
    {
    case AccessResult::Success:
        return value;
    case AccessResult::DoNotExists:
    case AccessResult::ArrayIndexOutOfBounds:
    case AccessResult::InvalidDimensionCount:
    case AccessResult::DimensionMismatch:
    case AccessResult::TypeMismatch:
        throw std::exception{}; // TODO
    }
    throw std::runtime_error{"unreachable"};
}

void ArrayExpr::setValue(const Value& value, Variables& variables) const
{
    auto indicies {evaluateIndicies(variables)};
    auto [oldValue, accessResult] = variables.getArrayValue(m_varName, indicies.begin(), indicies.begin() + m_indicies.size());
    switch (accessResult)
    {
    case Variables::AccessResult::DoNotExists:
        throw S840D_Alarm{12550};
    case Variables::AccessResult::ArrayIndexOutOfBounds:
        throw S840D_Alarm{17020};
    default:
        ;
    }

    variables.setArrayValue(m_varName, assignCast(value, getValueType(oldValue)),
                            indicies.begin(), indicies.begin() + m_indicies.size());
}

void ArrayExpr::setValues(const ArrayInitializer& values, Variables& variables) const
{
    // TODO implement

    // stub
    if (values.numberOfElements() == 1)
        setValue(values.getAndEvaluate(0, variables), variables);
    else
        throw S840D_Alarm{14130}; // too many initialization values given
}

std::array<s840d_int_t, 3> ArrayExpr::evaluateIndicies(Variables& variables) const
{
    std::array<s840d_int_t, 3> indiciesEvaluated;
    for (size_t i {0}; i < m_indicies.size(); i++)
    {
        try
        {
            s840d_int_t index {assignCastInt(m_indicies[i]->evaluate(variables))};//may throw
            indiciesEvaluated[i] = index;
        }
        catch (const S840D_Alarm& /*alarm*/)
        {
            //if (alarm.getAlarmCode() == 12150) //operation not compatible with data type
            throw S840D_Alarm{12410};//incorrect index type
        }
    }

    return indiciesEvaluated;
}

UnaryOpExpr::UnaryOpExpr(std::unique_ptr<Expr> arg, UnaryOpExpr::UnaryOp op)
    : m_arg(std::move(arg)),
      m_op(op)
{}

Value UnaryOpExpr::evaluate(Variables& variables) const
{
    switch (m_op)
    {
    case UMINUS:
        return negate(m_arg->evaluate(variables));
    case NOT:
        return logicalNot(m_arg->evaluate(variables));
    case BITWISE_NOT:
        return bitwiseNot(m_arg->evaluate(variables));
    }
    throw std::runtime_error{"unreachable"};
}

ArithmeticFunc1ArgExpr::ArithmeticFunc1ArgExpr(std::unique_ptr<Expr> arg, ArithmeticFunc1Arg op)
    : m_arg(std::move(arg)),
      m_op(op)
{}

ArithmeticFunc1ArgExpr::ArithmeticFunc1Arg ArithmeticFunc1ArgExpr::enumFromStr(const std::string& str)
{
    auto it = std::find(strings.begin(), strings.end(), str);
    if (it != strings.end())
        return static_cast<ArithmeticFunc1Arg>(it - strings.begin());

    return static_cast<ArithmeticFunc1Arg>(-1);
}

Value ArithmeticFunc1ArgExpr::evaluate(Variables& variables) const
{
    auto valOpt = convertToReal(m_arg->evaluate(variables));
    if (!valOpt)
        throw S840D_Alarm{12150};

    double val {*valOpt};
    switch (m_op)
    {
    case SIN:
        return std::sin(glm::radians(val));
    case COS:
        return std::cos(glm::radians(val));
    case TAN:
        return std::tan(glm::radians(val)); // TODO overflow check
    case ASIN:
        return glm::degrees(std::asin(val));
    case ACOS:
        return glm::degrees(std::acos(val));
    case SQRT:
        return std::sqrt(val); // TODO overflow check
    case ABS:
        return std::abs(val);
    case POT:
        return val * val; // TODO overflow check
    case TRUNC:
        return std::trunc(val);
    case ROUND:
        return std::round(val); // TODO overflow check
    case LN:
        return std::log(val); // TODO overflow check
    case EXP:
        return std::exp(val); // TODO overflow check
    }
    throw std::runtime_error{"unreachable"};
}

ArithmeticFunc2ArgExpr::ArithmeticFunc2ArgExpr(std::unique_ptr<Expr> arg1, std::unique_ptr<Expr> arg2, ArithmeticFunc2Arg op)
    : m_arg1(std::move(arg1)),
      m_arg2(std::move(arg2)),
      m_op(op)
{}

ArithmeticFunc2ArgExpr::ArithmeticFunc2Arg ArithmeticFunc2ArgExpr::enumFromStr(const std::string& str)
{
    auto it = std::find(strings.begin(), strings.end(), str);
    if (it != strings.end())
        return static_cast<ArithmeticFunc2Arg>(it - strings.begin());

    return static_cast<ArithmeticFunc2Arg>(-1);
}

Value ArithmeticFunc2ArgExpr::evaluate(Variables& variables) const
{
    auto valOpt1 = convertToReal(m_arg1->evaluate(variables));
    auto valOpt2 = convertToReal(m_arg2->evaluate(variables));
    if (!valOpt1 || !valOpt2)
        throw S840D_Alarm{12150};

    auto val1 {*valOpt1};
    auto val2 {*valOpt2};
    switch (m_op)
    {
    case ATAN2:
        return glm::degrees(std::atan2(val1, val2));
    case MINVAL:
        return std::min(val1, val2);
    case MAXVAL:
        return std::max(val1, val2);
    }
    throw std::runtime_error("unreachable");
}
