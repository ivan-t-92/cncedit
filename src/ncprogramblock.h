#ifndef NCPROGRAMBLOCK_H
#define NCPROGRAMBLOCK_H

#include "value.h"
#include "expr.h"
#include "variables.h"
#include "s840d_def.h"

#include <string>
#include <memory>
#include <vector>

class BlockContentVisitor;

class BlockContent
{
public:
    BlockContent() = default;
    virtual ~BlockContent() = default;
    BlockContent(const BlockContent&) = delete;
    BlockContent& operator=(const BlockContent&) = delete;
    BlockContent(const BlockContent&&) = delete;
    BlockContent& operator=(const BlockContent&&) = delete;

    virtual void accept(BlockContentVisitor& visitor) = 0;
};

class AddressAssign : public BlockContent
{
public:
    enum CoordType : int { COORD_TYPE(DEF_TYPE_ENUM) DEFAULT };

    const std::string m_address;
    const std::unique_ptr<Expr> m_expr;
    const CoordType m_coordType;

    explicit AddressAssign(std::string address, std::unique_ptr<Expr> expr, CoordType coordType = DEFAULT);
    static CoordType enumFromStr(const std::string& typeStr);
    void accept(BlockContentVisitor& visitor) override;
private:
    static constexpr std::array stringNames { COORD_TYPE(DEF_TYPE_STRING) "" };
};

class LValueAssign : public BlockContent
{
public:
    const std::unique_ptr<LValueExpr> m_lvalueExpr;
    const std::unique_ptr<Expr> m_expr;
    explicit LValueAssign(std::unique_ptr<LValueExpr> lvalueExpr, std::unique_ptr<Expr> expr);
    void accept(BlockContentVisitor& visitor) override;
};

class ExtAddressAssign : public BlockContent
{
public:
    const std::string m_address;
    const std::unique_ptr<Expr> m_ext;
    const std::unique_ptr<Expr> m_expr;

    explicit ExtAddressAssign(std::string address,
                              std::unique_ptr<Expr> ext,
                              std::unique_ptr<Expr> expr);
    void accept(BlockContentVisitor& visitor) override;
};

class GCommand : public BlockContent
{
public:
    enum FuncType : int { G_COMMANDS(DEF_TYPE_ENUM) };
    const FuncType m_type;

    explicit GCommand(FuncType type);
    static FuncType enumFromStr(const std::string& typeStr);
    void accept(BlockContentVisitor& visitor) override;
private:
    static constexpr std::array stringNames { G_COMMANDS(DEF_TYPE_STRING) };
};

class GotoStmt : public BlockContent
{
public:
    enum GotoType : int { GOTO_KEYWORDS(DEF_TYPE_ENUM) };
    const GotoType m_type;
    const std::unique_ptr<Expr> m_expr;

    explicit GotoStmt(GotoType type, std::unique_ptr<Expr> expr);
    static GotoType enumFromStr(const std::string& typeStr);
    void accept(BlockContentVisitor& visitor) override;
private:
    static constexpr std::array stringNames { GOTO_KEYWORDS(DEF_TYPE_STRING) };
};

class ConditionalGotoStmt : public BlockContent
{
public:
    const std::unique_ptr<Expr> m_conditionExpr;
    const std::unique_ptr<GotoStmt> m_gotoStmt;
    std::unique_ptr<ConditionalGotoStmt> m_next {nullptr};

    explicit ConditionalGotoStmt(std::unique_ptr<Expr> conditionExpr, std::unique_ptr<GotoStmt> gotoStmt);
    void accept(BlockContentVisitor& visitor) override;
};

class ForStmt : public BlockContent
{
public:
    const std::unique_ptr<LValueAssign> m_assignment;
    const std::unique_ptr<Expr> m_expr;

    explicit ForStmt(std::unique_ptr<LValueAssign> assignment, std::unique_ptr<Expr> expr);
    void accept(BlockContentVisitor& visitor) override;
};

class EndForStmt : public BlockContent
{
public:
    void accept(BlockContentVisitor& visitor) override;
};

class IfStmt : public BlockContent
{
public:
    const std::unique_ptr<Expr> m_expr;

    explicit IfStmt(std::unique_ptr<Expr> expr);
    void accept(BlockContentVisitor& visitor) override;
};

class ElseStmt : public BlockContent
{
public:
    void accept(BlockContentVisitor& visitor) override;
};

class EndIfStmt : public BlockContent
{
public:
    void accept(BlockContentVisitor& visitor) override;
};

class DefStmt : public BlockContent
{
public:
    struct Def
    {
        const std::string varName;
        const Value initValue;
    };
    struct ArrayDef
    {
        const std::string varName;
        const std::vector<s840d_int_t> arrayDimensions;
        //TODO array initializer here
    };
    const std::vector<Def> m_defs;
    const std::vector<ArrayDef> m_arrayDefs;
    const ValueType m_type;

    explicit DefStmt(std::vector<Def> defs, std::vector<ArrayDef> arrayDefs, ValueType type);

    void accept(BlockContentVisitor& visitor) override;
};

class BlockContentVisitor
{
public:
    BlockContentVisitor() = default;
    virtual ~BlockContentVisitor() = default;
    BlockContentVisitor(const BlockContentVisitor&) = delete;
    BlockContentVisitor& operator=(const BlockContentVisitor&) = delete;
    BlockContentVisitor(const BlockContentVisitor&&) = delete;
    BlockContentVisitor& operator=(const BlockContentVisitor&&) = delete;

    virtual void visit(AddressAssign& addressAssign) = 0;
    virtual void visit(ExtAddressAssign& extAddressAssign) = 0;
    virtual void visit(LValueAssign& lvalueAssign) = 0;
    virtual void visit(GCommand& func) = 0;
    virtual void visit(GotoStmt& gotoStmt) = 0;
    virtual void visit(ConditionalGotoStmt& gotoStmt) = 0;
    virtual void visit(ForStmt& gotoStmt) = 0;
    virtual void visit(EndForStmt& gotoStmt) = 0;
    virtual void visit(IfStmt& ifStmt) = 0;
    virtual void visit(ElseStmt& elseStmt) = 0;
    virtual void visit(EndIfStmt& endIfStmt) = 0;
    virtual void visit(DefStmt& defStmt) = 0;
};

struct BlockNumber
{
    enum BlockNumberType { Regular, Main };
    std::string m_number;
    BlockNumberType m_type {Regular};
};

struct NCProgramBlock
{
    std::vector<BlockContent*> blockContent;
    BlockNumber blockNumber;
    std::string label;
    union
    {
        int skipLevel {-1}; // for normal blocks
        int nestingLevel; // for control structures (for, if, loop etc)
    };
};



#endif // NCPROGRAMBLOCK_H
