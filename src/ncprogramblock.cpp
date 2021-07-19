#include "ncprogramblock.h"

#include <algorithm>

AddressAssign::AddressAssign(std::string address, std::unique_ptr<Expr> expr, CoordType coordType)
    : m_address(std::move(address)),
      m_expr(std::move(expr)),
      m_coordType(coordType)
{}

AddressAssign::CoordType AddressAssign::enumFromStr(const std::string& typeStr)
{
    auto it = std::find(stringNames.begin(), stringNames.end(), typeStr);
    if (it != stringNames.end())
        return static_cast<CoordType>(it - stringNames.begin());

    return static_cast<CoordType>(-1);
}

void AddressAssign::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

ExtAddressAssign::ExtAddressAssign(std::string address, std::unique_ptr<Expr> ext, std::unique_ptr<Expr> expr)
    : m_address(std::move(address)),
      m_ext(std::move(ext)),
      m_expr(std::move(expr))
{}

void ExtAddressAssign::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

GCommand::GCommand(FuncType type)
    : m_type(type)
{}

void GCommand::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

GCommand::FuncType GCommand::enumFromStr(const std::string& typeStr)
{
    auto it = std::find(stringNames.begin(), stringNames.end(), typeStr);
    if (it != stringNames.end())
        return static_cast<FuncType>(it - stringNames.begin());

    return static_cast<FuncType>(-1);
}

GotoStmt::GotoStmt(GotoType type, std::unique_ptr<Expr> expr)
    : m_type(type),
      m_expr(std::move(expr))
{}

GotoStmt::GotoType GotoStmt::enumFromStr(const std::string& typeStr)
{
    auto it = std::find(stringNames.begin(), stringNames.end(), typeStr);
    if (it != stringNames.end())
        return static_cast<GotoType>(it - stringNames.begin());

    return static_cast<GotoType>(-1);
}

void GotoStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

ConditionalGotoStmt::ConditionalGotoStmt(std::unique_ptr<Expr> conditionExpr, std::unique_ptr<GotoStmt> gotoStmt)
    : m_conditionExpr(std::move(conditionExpr)),
      m_gotoStmt(std::move(gotoStmt))
{}

void ConditionalGotoStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

LValueAssign::LValueAssign(std::unique_ptr<LValueExpr> lvalueExpr, std::unique_ptr<Expr> expr)
    : m_lvalueExpr(std::move(lvalueExpr)),
      m_expr(std::move(expr))
{}

void LValueAssign::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

ForStmt::ForStmt(std::unique_ptr<LValueAssign> assignment, std::unique_ptr<Expr> expr)
    : m_assignment(std::move(assignment)),
      m_expr(std::move(expr))
{}

void ForStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

void EndForStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

IfStmt::IfStmt(std::unique_ptr<Expr> expr)
    : m_expr(std::move(expr))
{}

void IfStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

void ElseStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

void EndIfStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}

DefStmt::DefStmt(std::vector<Def> defs, std::vector<ArrayDef> arrayDefs, ValueType type)
    : m_defs(std::move(defs)),
      m_arrayDefs(std::move(arrayDefs)),
      m_type(type)
{}

void DefStmt::accept(BlockContentVisitor& visitor)
{
    visitor.visit(*this);
}
