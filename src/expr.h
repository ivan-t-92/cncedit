#ifndef EXPR_H
#define EXPR_H

#include "value.h"
#include "s840d_def.h"

#include <array>
#include <memory>
#include <vector>
#include <string>

class Variables;

class Expr
{
public:
    Expr() = default;
    virtual ~Expr() = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(const Expr&&) = delete;
    Expr& operator=(const Expr&&) = delete;

    virtual Value evaluate(Variables& variables) const = 0;
};

class LiteralExpr : public Expr
{
public:
    explicit LiteralExpr(Value value);
    Value evaluate(Variables& variables) const override;

    const Value m_value;
};

class BinaryOpExpr : public Expr
{
public:
    enum BinaryOp
    {
        ADD,
        SUB,
        MUL,
        DIV_FP,
        DIV_INT,
        MOD,
        AND,
        OR,
        XOR,
        EQUAL,
        NOTEQUAL,
        GREATER,
        LESS,
        GREATER_OR_EQUAL,
        LESS_OR_EQUAL,
        BITWISE_AND,
        BITWISE_OR,
        BITWISE_XOR,
    };

    explicit BinaryOpExpr(std::unique_ptr<Expr> lhs,
                          std::unique_ptr<Expr> rhs,
                          BinaryOp op);
    Value evaluate(Variables& variables) const override;
    static Value evaluate(Expr* lhs, Expr* rhs, BinaryOp op, Variables& variables);

    const std::unique_ptr<Expr> m_lhs;
    const std::unique_ptr<Expr> m_rhs;
    const BinaryOp m_op;
};

class ArrayInitializer
{
public:
    ArrayInitializer() = default;
    virtual ~ArrayInitializer() = default;
    ArrayInitializer(const ArrayInitializer&) = delete;
    ArrayInitializer& operator=(const ArrayInitializer&) = delete;
    ArrayInitializer(const ArrayInitializer&&) = delete;
    ArrayInitializer& operator=(const ArrayInitializer&&) = delete;

    virtual s840d_int_t numberOfElements() const = 0;
    virtual Value getAndEvaluate(s840d_int_t index, Variables& variables) const = 0;
};

class SetArrayInitializer : public ArrayInitializer
{
public:
    explicit SetArrayInitializer(std::vector<std::unique_ptr<Expr>> values);

    s840d_int_t numberOfElements() const override;
    Value getAndEvaluate(s840d_int_t index, Variables& variables) const override;

    const std::vector<std::unique_ptr<Expr>> m_values;
};

class RepArrayInitializer : public ArrayInitializer
{
public:
    explicit RepArrayInitializer(std::unique_ptr<Expr> value, s840d_int_t nElements);

    s840d_int_t numberOfElements() const override;
    Value getAndEvaluate(s840d_int_t index, Variables& variables) const override;

    const std::unique_ptr<Expr> m_value;
    const s840d_int_t m_nElements;
};

class LValueExpr : public Expr
{
public:
    virtual void setValue(const Value& value, Variables& variables) const = 0;
    virtual void setValues(const ArrayInitializer& values, Variables& variables) const = 0;
};

class VariableExpr : public LValueExpr
{
public:
    explicit VariableExpr(std::string varName);
    Value evaluate(Variables& variables) const override;
    void setValue(const Value& value, Variables& variables) const override;
    void setValues(const ArrayInitializer& values, Variables& variables) const override;

    const std::string m_varName;
};

class ArrayExpr : public LValueExpr
{
public:
    explicit ArrayExpr(std::string varName, std::vector<Expr*> indicies);
    ~ArrayExpr();
    Value evaluate(Variables& variables) const override;
    void setValue(const Value& value, Variables& variables) const override;
    void setValues(const ArrayInitializer& values, Variables& variables) const override;

    const std::string m_varName;
    const std::vector<Expr*> m_indicies;

private:
    std::array<s840d_int_t, 3> evaluateIndicies(Variables& variables) const;
};

class UnaryOpExpr : public Expr
{
public:
    enum UnaryOp
    {
        UMINUS,
        NOT,
        BITWISE_NOT
    };

    explicit UnaryOpExpr(std::unique_ptr<Expr> arg, UnaryOp op);
    Value evaluate(Variables& variables) const override;

    const std::unique_ptr<Expr> m_arg;
    const UnaryOp m_op;
};

class ArithmeticFunc1ArgExpr : public Expr
{
public:
    enum ArithmeticFunc1Arg : int
    {
        ARITHMETIC_FUNC_1ARG(DEF_TYPE_ENUM)
    };

    explicit ArithmeticFunc1ArgExpr(std::unique_ptr<Expr> arg, ArithmeticFunc1Arg op);
    static ArithmeticFunc1Arg enumFromStr(const std::string& str);
    Value evaluate(Variables& variables) const override;

    const std::unique_ptr<Expr> m_arg;
    const ArithmeticFunc1Arg m_op;

private:
    static constexpr std::array strings {ARITHMETIC_FUNC_1ARG(DEF_TYPE_STRING)};
};

class ArithmeticFunc2ArgExpr : public Expr
{
public:
    enum ArithmeticFunc2Arg : int
    {
        ARITHMETIC_FUNC_2ARG(DEF_TYPE_ENUM)
    };

    explicit ArithmeticFunc2ArgExpr(std::unique_ptr<Expr> arg1,
                                    std::unique_ptr<Expr> arg2,
                                    ArithmeticFunc2Arg op);
    static ArithmeticFunc2Arg enumFromStr(const std::string& str);
    Value evaluate(Variables& variables) const override;

    const std::unique_ptr<Expr> m_arg1;
    const std::unique_ptr<Expr> m_arg2;
    const ArithmeticFunc2Arg m_op;

private:
    static constexpr std::array strings {ARITHMETIC_FUNC_2ARG(DEF_TYPE_STRING)};
};

#endif // EXPR_H
