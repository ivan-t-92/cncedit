#include "parser.h"
#include "value.h"
#include "s840d_def.h"
#include "s840d_alarm.h"
#include "util.h"

#include <parsertl/lookup.hpp>
#include <parsertl/debug.hpp>


struct ParserContext
{
    NCProgramBlock& currentBlock;
    int& currentNestingLevel;
    std::vector<std::any>& stack;
    parsertl::state_machine& gsm;
    parsertl::match_results& results;
    parsertl::token<lexertl::citerator>::token_vector& productions;

    auto& token(std::size_t index) const
    {
        return results.dollar(gsm, index, productions);
    }
};

struct Token
{
    const char* name;
    const std::vector<const char*> patterns;
};

const std::vector<Token> tokensLeftAssoc {
    {"'+'",              {"[+]"} },
    {"'-'",              {"-"} },
    {"'*'",              {"[*]"} },
    {"'/'",              {"[/]"} },
    {"DIV",              {"DIV"} },
    {"MOD",              {"MOD"} },
    {"EQ",               {"=="} },
    {"NE",               {"<>"} },
    {"GT",               {">"} },
    {"LT",               {"<"} },
    {"GE",               {">="} },
    {"LE",               {"<="} },
    {"AND",              {"AND"} },
    {"OR",               {"OR"} },
    {"XOR",              {"XOR"} },
    {"NOT",              {"NOT"} },
    {"B_AND",            {"B_AND"} },
    {"B_OR",             {"B_OR"} },
    {"B_XOR",            {"B_XOR"} },
    {"B_NOT",            {"B_NOT"} },
};

const std::vector<Token> tokens {
    {"INTEGER",                {"\\d+"} },
    {"INTEGER_BIN",            {R"('B[01]+')"} },
    {"INTEGER_HEX",            {R"('H[\dA-F]+')"} },
    {"FLOAT",                  {R"(\d+\.\d*)",
                                R"(\d*\.\d+)"} },
    {"FLOAT_EX",               {R"(\d+\.\d*EX[+-]?\d+)",
                                R"(\d+EX[+-]?\d+)",
                                R"(\d*\.\d+EX[+-]?\d+)"} },
    {"STRING_LITERAL",         {R"(["][^"\n]*["])"} },
    {"'N'",                    {"N"} },
    {"'G'",                    {"G"} },
    {"'R'",                    {"R",
                                "$R"} },
    {"':'",                    {"[:]"} },
    {"'D'",                    {"D"} },
    {"ADDRESS_LETTER_EXT_1",   {"[ABCEFIJKUVWXYZ]"} },
    {"ADDRESS_LETTER_EXT_2",   {"[RL]"} },
    {"ADDRESS_LETTER_EXT_AUX", {"[MSHT]"} },
    {"COORD_TYPE",             {COORD_TYPE(DEF_TYPE_STRING)} },
    {"IF",                     {"IF"} },
    {"ELSE",                   {"ELSE"} },
    {"ENDIF",                  {"ENDIF"} },
    {"FOR",                    {"FOR"} },
    {"TO",                     {"TO"} },
    {"ENDFOR",                 {"ENDFOR"} },
    {"DEF",                    {"DEF"} },
    {"PROC",                   {"PROC"} },
    {"RET",                    {"RET"} },
    {"ADDRESS_NO_AX_EXT",
        {"OVR", "OVRRAP", "SPOS", "SCC", "SPOSA", "CR", "TURN"} },
    {"TYPE_STRING",            {"STRING"} },
    {"TYPE_OTHER",             {"INT", "REAL", "BOOL", "CHAR"} },
    {"ARITHMETIC_FUNC",        {ARITHMETIC_FUNC_1ARG(DEF_TYPE_STRING)
                                ARITHMETIC_FUNC_2ARG(DEF_TYPE_STRING)}},
    {"FUNC",                   {G_COMMANDS(DEF_TYPE_STRING)}},
    {"GOTO",                   {GOTO_KEYWORDS(DEF_TYPE_STRING)}},
    {"IDENTIFIER",             {R"([a-z_]{2}\w*)"}},
    {"EOL",                    {R"([\n\r])"} },
    {"'='",                    {"="} },
    {"'['",                    {R"(\[)"} },
    {"']'",                    {R"(\])"} },
    {"'('",                    {R"(\()"} },
    {"')'",                    {R"(\))"} },
    {"','",                    {"[,]"} },
};

static void createBinary(ParserContext& context, BinaryOpExpr::BinaryOp binOp)
{
    Expr* rhs {std::any_cast<Expr*>(context.stack.back())};
    context.stack.pop_back();
    Expr* lhs {std::any_cast<Expr*>(context.stack.back())};
    context.stack.pop_back();

    context.stack.push_back(std::make_any<Expr*>(
        new BinaryOpExpr(std::unique_ptr<Expr>(lhs), std::unique_ptr<Expr>(rhs), binOp)));
}

static void createUnary(ParserContext& context, UnaryOpExpr::UnaryOp unaryOp)
{
    Expr* expr {std::any_cast<Expr*>(context.stack.back())};
    context.stack.pop_back();

    context.stack.push_back(std::make_any<Expr*>(
        new UnaryOpExpr(std::unique_ptr<Expr>(expr), unaryOp)));
}

static void addLexerRules(const std::vector<Token>& tokens,
                          const parsertl::rules& grules,
                          lexertl::rules& lrules)
{
    for (auto& token : tokens)
    {
        auto id = grules.token_id(token.name);
        for (auto pattern : token.patterns)
        {
            lrules.push(pattern, id);
        }
    }
}

static void checkControlStructureBlock (const NCProgramBlock& block)
{
    if (!block.label.empty() || block.skipLevel >= 0)
        throw S840D_Alarm{12630}; //skip ID/label in control structure not allowed
}

Parser::Parser()
{

    constexpr int initialCapacity {32};
    m_productions.reserve(initialCapacity);
    m_stack.reserve(initialCapacity);
    // parser setup
    for (auto& token : tokens)
    {
        m_grules.token(token.name);
    }
    m_grules.left("EQ NE GT LT GE LE");
    m_grules.left("OR");
    m_grules.left("XOR");
    m_grules.left("AND");
    m_grules.left("B_OR");
    m_grules.left("B_XOR");
    m_grules.left("B_AND");
    m_grules.left("'+' '-'");
    m_grules.left("'*' '/' DIV MOD");
    m_grules.left("NOT B_NOT");
    //grules.precedence("UMINUS");

    m_grules.push("start", "block");

    m_grules.push("block", "block_content_opt eol_opt");
    m_grules.push("eol_opt", "EOL | %empty");


    using expr_opt_t = std::optional<Expr*>;

    m_semanticActionMap[ m_grules.push("block_content_opt", "words")] = [](ParserContext& context)
    {
        std::vector<BlockContent*>* words {std::any_cast<std::vector<BlockContent*>*>(context.stack.back())};
        context.currentBlock.blockContent = std::move(*words);
        delete words;
    };
    m_semanticActionMap[ m_grules.push("block_content_opt", "stmt")] = [](ParserContext& context)
    {
        context.currentBlock.blockContent.push_back(std::any_cast<BlockContent*>(context.stack.back()));
        context.stack.pop_back();
    };

    m_grules.push("block_content_opt", "%empty");

    m_semanticActionMap[ m_grules.push("words", "words word")] = [](ParserContext& context)
    {
        BlockContent* word {std::any_cast<BlockContent*>(context.stack.back())};
        context.stack.pop_back();
        std::vector<BlockContent*>* words {std::any_cast<std::vector<BlockContent*>*>(context.stack.back())};
        words->push_back(word);
    };
    m_semanticActionMap[ m_grules.push("words", "word")] = [](ParserContext& context)
    {
        BlockContent* word {std::any_cast<BlockContent*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<std::vector<BlockContent*>*>(
            new std::vector<BlockContent*>{word}));
    };

    auto literalAddressAssign = [](ParserContext& context)
    {
        Value v {std::any_cast<Value>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new AddressAssign(context.token(0).str(),
                              std::make_unique<LiteralExpr>(v))));
    };
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_LETTER_EXT_1 num")] = literalAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_LETTER_EXT_AUX num")] = literalAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "address_letter '-' num")] = [](ParserContext& context)
    {
        Value v {std::any_cast<Value>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new AddressAssign(context.token(0).str(),
                              std::make_unique<UnaryOpExpr>(
                                  std::make_unique<LiteralExpr>(v), UnaryOpExpr::UMINUS))));
    };
    m_semanticActionMap[ m_grules.push("word", "address_letter '+' num")] = literalAddressAssign;
    auto exprAddressAssign = [](ParserContext& context)
    {
        std::unique_ptr<Expr> v {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new AddressAssign(context.token(0).str(),
                              std::move(v))));
    };
    auto exprAddressAssignCoordType = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        auto coordTypeStr {context.token(2).str()};
        to_upper(coordTypeStr);
        auto coordType {AddressAssign::enumFromStr(coordTypeStr)};
        context.stack.push_back(std::make_any<BlockContent*>(
            new AddressAssign(context.token(0).str(), std::move(expr), coordType)));
    };
    m_semanticActionMap[ m_grules.push("word", "address_letter '=' expr")] = exprAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "address_letter '=' COORD_TYPE '(' expr ')'")] = exprAddressAssignCoordType;
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_NO_AX_EXT '=' expr")] = exprAddressAssign;
    auto exprExtAddressAssign = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new AddressAssign(context.token(0).str() + context.token(1).str(), std::move(expr))));
    };
    auto exprExtAddressAssignCoordType = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        auto coordTypeStr {context.token(3).str()};
        to_upper(coordTypeStr);
        auto coordType {AddressAssign::enumFromStr(coordTypeStr)};
        context.stack.push_back(std::make_any<BlockContent*>(
            new AddressAssign(context.token(0).str() + context.token(1).str(), std::move(expr), coordType)));
    };
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_LETTER_EXT_1 INTEGER '=' expr")] = exprExtAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_LETTER_EXT_1 INTEGER '=' COORD_TYPE '(' expr ')'")] = exprExtAddressAssignCoordType;
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_LETTER_EXT_AUX INTEGER '=' expr")] = exprExtAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "ADDRESS_LETTER_EXT_AUX '[' expr ']' '=' expr")] = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        std::unique_ptr<Expr> extExpr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new ExtAddressAssign(context.token(0).str(), std::move(extExpr), std::move(expr))));
    };
    auto integerAddressAssign = [](ParserContext& context)
    {
        try
        {
            int i = std::stoi(context.token(1).str());
            context.stack.push_back(std::make_any<BlockContent*>(
                new AddressAssign(context.token(0).str(),
                                  std::make_unique<LiteralExpr>(Value{i}))));
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm(12470);
        }
    };
    m_semanticActionMap[ m_grules.push("word", "'D' INTEGER")] = integerAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "'D' '=' expr")] = exprAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "'G' INTEGER")] = integerAddressAssign;
    m_semanticActionMap[ m_grules.push("word", "'G' '[' INTEGER ']' '=' expr")] = [](ParserContext& context)
    {
        try
        {
            std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
            context.stack.pop_back();
            int i = std::stoi(context.token(2).str());
            context.stack.push_back(std::make_any<BlockContent*>(
                new ExtAddressAssign(context.token(0).str(),
                                     std::make_unique<LiteralExpr>(Value{i}),
                                     std::move(expr))));
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12160};
        }
    };
    m_grules.push("word", "assignment");
    m_semanticActionMap[ m_grules.push("assignment", "IDENTIFIER '=' expr")] = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        auto id {context.token(0).str()};

        context.stack.push_back(std::make_any<BlockContent*>(
            new LValueAssign(std::make_unique<VariableExpr>(id),
                             std::move(expr))));
    };
    m_semanticActionMap[ m_grules.push("assignment", "r_param '=' expr")] = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        auto i {std::any_cast<int>(context.stack.back())};
        context.stack.pop_back();

        context.stack.push_back(std::make_any<BlockContent*>(
            new LValueAssign(std::make_unique<ArrayExpr>("R", std::vector<Expr*>{new LiteralExpr(i)}),
                             std::move(expr))));
    };
    m_semanticActionMap[ m_grules.push("assignment", "array_expr '=' expr")] = [](ParserContext& context)
    {
        std::unique_ptr<Expr> expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        std::unique_ptr<ArrayExpr> arrayExpr {static_cast<ArrayExpr*>(std::any_cast<Expr*>(context.stack.back()))};
        context.stack.pop_back();

        context.stack.push_back(std::make_any<BlockContent*>(
            new LValueAssign(std::move(arrayExpr), std::move(expr))));
    };
    m_semanticActionMap[ m_grules.push("word", "FUNC")] = [](ParserContext& context)
    {
        std::string funcStr = context.token(0).str();
        to_upper(funcStr);

        auto func = GCommand::enumFromStr(funcStr);
        if (func < 0)
            throw std::runtime_error("can not construct object from " + funcStr);

        context.stack.push_back(std::make_any<BlockContent*>(
            new GCommand(func)));
    };

    m_grules.push("address_letter", "ADDRESS_LETTER_EXT_1 | ADDRESS_LETTER_EXT_2 | ADDRESS_LETTER_EXT_AUX");

    m_semanticActionMap[ m_grules.push("num", "INTEGER")] = [](ParserContext& context)
    {
        auto& token = context.token(0);
        try
        {
            auto i = std::stoi(token.str());
            context.stack.push_back(std::make_any<Value>(i));
        }
        catch (std::out_of_range&)
        {
            auto d {str_to_double_noexp(token.first, token.second)};
            if (!d.has_value())
                throw S840D_Alarm{12160};
            context.stack.push_back(std::make_any<Value>(d.value()));
        }
    };
    m_semanticActionMap[ m_grules.push("num", "INTEGER_BIN")] = [](ParserContext& context)
    {
        auto& token = context.token(0);
        std::string str {token.first + 2, token.second};
        try
        {
            auto i = std::stoi(str, nullptr, 2);
            context.stack.push_back(std::make_any<Value>(i));
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12160};
        }
    };
    m_semanticActionMap[ m_grules.push("num", "INTEGER_HEX")] = [](ParserContext& context)
    {
        auto& token {context.token(0)};
        std::string str {token.first + 2, token.second};
        try
        {
            auto i = std::stoi(str, nullptr, 16);
            context.stack.push_back(std::make_any<Value>(i));
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12160};
        }
    };
    m_semanticActionMap[ m_grules.push("num", "FLOAT")] = [](ParserContext& context)
    {
        auto& token {context.token(0)};

        auto d {str_to_double_noexp(token.first, token.second)};
        if (!d.has_value())
            throw S840D_Alarm{12160};
        context.stack.push_back(std::make_any<Value>(d.value()));
    };
    m_semanticActionMap[ m_grules.push("num", "FLOAT_EX")] = [](ParserContext& context)
    {
        auto& token = context.token(0);
        auto d {str_to_double_s840d_exp(token.first, token.second)};
        if (!d.has_value())
            throw S840D_Alarm{12160};
        context.stack.push_back(std::make_any<Value>(d.value()));
    };
    m_grules.push("literal", "num");
    m_semanticActionMap[ m_grules.push("literal", "STRING_LITERAL")] = [](ParserContext& context)
    {
        auto& token {context.token(0)};
        context.stack.push_back(std::make_any<Value>(std::string(token.first + 1, token.second - 1)));
    };

    m_semanticActionMap[ m_grules.push("expr", "literal")] = [](ParserContext& context)
    {
        Value v {std::any_cast<Value>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<Expr*>(new LiteralExpr(v)));
    };
//    m_semanticActionMap[ m_grules.push("expr", "STRING_LITERAL")] = [](ParserContext& context)
//    {
//        auto& strToken {context.results.dollar(context.gsm, 0, context.productions)};
//        context.stack.push_back(std::make_any<Expr*>(
//            new LiteralExpr(std::string(strToken.first + 1, strToken.second - 1))));
//    };
    m_semanticActionMap[ m_grules.push("expr", "IDENTIFIER")] = [](ParserContext& context)
    {
        context.stack.push_back(std::make_any<Expr*>(new VariableExpr(context.token(0).str())));
    };
    m_semanticActionMap[ m_grules.push("expr", "r_param")] = [](ParserContext& context)
    {
        int i {std::any_cast<int>(context.stack.back())};
        context.stack.pop_back();

        context.stack.push_back(std::make_any<Expr*>(
            new ArrayExpr("R", std::vector<Expr*>{new LiteralExpr(i)})));
    };
    m_grules.push("expr", "'(' expr ')'");
    m_grules.push("expr", "array_expr");
    m_semanticActionMap[ m_grules.push("expr", "expr '+' expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::ADD);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr '-' expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::SUB);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr '*' expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::MUL);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr '/' expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::DIV_FP);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr DIV expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::DIV_INT);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr MOD expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::MOD);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr AND expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::AND);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr OR expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::OR);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr XOR expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::XOR);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr B_AND expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::BITWISE_AND);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr B_OR expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::BITWISE_OR);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr B_XOR expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::BITWISE_XOR);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr EQ expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::EQUAL);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr NE expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::NOTEQUAL);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr GT expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::GREATER);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr LT expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::LESS);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr GE expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::GREATER_OR_EQUAL);
    };
    m_semanticActionMap[ m_grules.push("expr", "expr LE expr")] = [](ParserContext& context)
    {
        createBinary(context, BinaryOpExpr::LESS_OR_EQUAL);
    };
    m_semanticActionMap[ m_grules.push("expr", "'-' expr")] = [](ParserContext& context)
    {
        createUnary(context, UnaryOpExpr::UMINUS);
    };
    m_semanticActionMap[ m_grules.push("expr", "NOT expr")] = [](ParserContext& context)
    {
        createUnary(context, UnaryOpExpr::NOT);
    };
    m_semanticActionMap[ m_grules.push("expr", "B_NOT expr")] = [](ParserContext& context)
    {
        createUnary(context, UnaryOpExpr::BITWISE_NOT);
    };
    m_grules.push("expr", "'+' expr");
    m_semanticActionMap[ m_grules.push("expr", "ARITHMETIC_FUNC '(' expr_opt_list ')'")] = [](ParserContext& context)
    {
        // TODO rewrite: first determine argument number from the func name, then compare with the list length

        auto funcStr {context.token(0).str()};
        to_upper(funcStr);

        std::vector<expr_opt_t>* expr_opt_list_ptr;
        try
        {
            expr_opt_list_ptr = std::any_cast<std::vector<expr_opt_t>*>(context.stack.back());
        }
        catch (std::bad_any_cast& e)
        {
            throw S840D_Alarm{14020};
        }
        std::unique_ptr<std::vector<expr_opt_t>> expr_opt_list {expr_opt_list_ptr};
        context.stack.pop_back();
        switch (expr_opt_list->size())
        {
        case 1:
        {
            auto func = ArithmeticFunc1ArgExpr::enumFromStr(funcStr);
            if (func < 0)
                throw S840D_Alarm{14020};

            if (!(*expr_opt_list)[0].has_value())
                throw S840D_Alarm{14020};

            context.stack.push_back(std::make_any<Expr*>(
                new ArithmeticFunc1ArgExpr(std::unique_ptr<Expr>{(*expr_opt_list)[0].value()}, func)));
            break;
        }
        case 2:
        {
            auto func = ArithmeticFunc2ArgExpr::enumFromStr(funcStr);
            if (func < 0)
                throw S840D_Alarm{14020};

            if (!(*expr_opt_list)[1].has_value())
                throw S840D_Alarm{14020};

            context.stack.push_back(std::make_any<Expr*>(
                new ArithmeticFunc2ArgExpr((*expr_opt_list)[0].has_value() ?
                                                std::unique_ptr<Expr>{(*expr_opt_list)[0].value()} :
                                                std::unique_ptr<Expr>{new LiteralExpr(createDefaultValue(ValueType::INT))},
                                           std::unique_ptr<Expr>{(*expr_opt_list)[1].value()}, func)));
            break;
        }
        default:
            throw S840D_Alarm{14020};
        }
    };
    m_semanticActionMap[ m_grules.push("r_param", "'R' INTEGER")] = [](ParserContext& context)
    {
        try
        {
            int i = std::stoi(context.token(1).str());
            context.stack.emplace_back(i);
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12160};
        }
    };
    m_semanticActionMap[ m_grules.push("array_expr", "IDENTIFIER '[' expr ']'")] = [](ParserContext& context)
    {
        Expr* expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<Expr*>(
            new ArrayExpr(context.token(0).str(),
                          std::vector<Expr*>{expr})));
    };
    m_semanticActionMap[ m_grules.push("array_expr", "IDENTIFIER '[' expr ',' expr ']'")] = [](ParserContext& context)
    {
        Expr* expr2 {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        Expr* expr1 {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<Expr*>(
            new ArrayExpr(context.token(0).str(),
                          std::vector<Expr*>{expr1, expr2})));
    };
    m_semanticActionMap[ m_grules.push("array_expr", "IDENTIFIER '[' expr ',' expr ',' expr ']'")] = [](ParserContext& context)
    {
        Expr* expr3 {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        Expr* expr2 {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        Expr* expr1 {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<Expr*>(
            new ArrayExpr(context.token(0).str(),
                          std::vector<Expr*>{expr1, expr2, expr3})));
    };

    m_semanticActionMap[ m_grules.push("expr_opt_list", "expr_opt_list ',' expr_opt")] = [](ParserContext& context)
    {
        auto expr_opt {std::any_cast<expr_opt_t>(context.stack.back())};
        context.stack.pop_back();
        auto exprList {std::any_cast<std::vector<expr_opt_t>*>(context.stack.back())};
        exprList->push_back(expr_opt);
    };
    m_semanticActionMap[ m_grules.push("expr_opt_list", "expr_opt")] = [](ParserContext& context)
    {
        auto expr {std::any_cast<expr_opt_t>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(new std::vector<expr_opt_t>{expr});
    };

    m_semanticActionMap[ m_grules.push("expr_opt", "expr")] = [](ParserContext& context)
    {
        auto expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(expr_opt_t{expr});
    };
    m_semanticActionMap[ m_grules.push("expr_opt", "%empty")] = [](ParserContext& context)
    {
        context.stack.push_back(expr_opt_t{});
    };

    m_grules.push("stmt", "conditional_goto_stmts | goto_stmt |"
                          "for_stmt | endfor_stmt |"
                          "if_stmt | else_stmt | endif_stmt | "
                          "def_stmt");

    m_semanticActionMap[ m_grules.push("conditional_goto_stmt", "IF expr goto_stmt")] = [](ParserContext& context)
    {
        auto gotoStmt {dynamic_cast<GotoStmt*>(std::any_cast<BlockContent*>(context.stack.back()))};
        context.stack.pop_back();
        auto expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();

        context.stack.push_back(std::make_any<BlockContent*>(
            new ConditionalGotoStmt(std::unique_ptr<Expr>{expr},
                                    std::unique_ptr<GotoStmt>{gotoStmt})));
    };

    m_semanticActionMap[ m_grules.push("conditional_goto_stmts", "conditional_goto_stmts conditional_goto_stmt")] = [](ParserContext& context)
    {
        auto gotoStmtNext {dynamic_cast<ConditionalGotoStmt*>(std::any_cast<BlockContent*>(context.stack.back()))};
        context.stack.pop_back();
        auto gotoStmt {dynamic_cast<ConditionalGotoStmt*>(std::any_cast<BlockContent*>(context.stack.back()))};
        gotoStmt->m_next.reset(gotoStmtNext);
    };

    m_grules.push("conditional_goto_stmts", "conditional_goto_stmt");

    m_semanticActionMap[ m_grules.push("goto_stmt", "GOTO expr")] = [](ParserContext& context)
    {
        auto expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        if (auto varExpr = dynamic_cast<VariableExpr*>(expr))
        {
            // TODO check for known variable identifiers (how???) and do not convert
            // to string (i.e. label target) if there is one
            expr = new LiteralExpr(varExpr->m_varName);
            delete varExpr;
        }

        auto keyword {context.token(0).str()};
        to_upper(keyword);

        context.stack.push_back(std::make_any<BlockContent*>(
            new GotoStmt(GotoStmt::enumFromStr(keyword),
                         std::unique_ptr<Expr>{expr})));
    };
    m_semanticActionMap[ m_grules.push("goto_stmt", "GOTO 'N' INTEGER")] = [](ParserContext& context)
    {
        auto keyword {context.token(0).str()};
        to_upper(keyword);

        std::unique_ptr<Expr> expr {
            new LiteralExpr{context.token(2).str()}};
        context.stack.push_back(std::make_any<BlockContent*>(
            new GotoStmt(GotoStmt::enumFromStr(keyword),
                         std::move(expr))));
    };
    m_semanticActionMap[ m_grules.push("for_stmt", "FOR assignment TO expr")] = [](ParserContext& context)
    {
        checkControlStructureBlock(context.currentBlock);

        context.currentNestingLevel++;
        context.currentBlock.nestingLevel = context.currentNestingLevel;

        auto expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        auto assignment {std::any_cast<BlockContent*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new ForStmt(std::unique_ptr<LValueAssign>(dynamic_cast<LValueAssign*>(assignment)),
                        std::unique_ptr<Expr>(expr))));
    };

    m_semanticActionMap[ m_grules.push("endfor_stmt", "ENDFOR")] = [](ParserContext& context)
    {
        checkControlStructureBlock(context.currentBlock);

        context.currentBlock.nestingLevel = context.currentNestingLevel;
        context.currentNestingLevel--;

        context.stack.push_back(std::make_any<BlockContent*>(new EndForStmt()));
    };

    m_semanticActionMap[ m_grules.push("if_stmt", "IF expr")] = [](ParserContext& context)
    {
        checkControlStructureBlock(context.currentBlock);

        context.currentNestingLevel++;
        context.currentBlock.nestingLevel = context.currentNestingLevel;

        auto expr {std::any_cast<Expr*>(context.stack.back())};
        context.stack.pop_back();
        context.stack.push_back(std::make_any<BlockContent*>(
            new IfStmt(std::unique_ptr<Expr>(expr))));
    };

    m_semanticActionMap[ m_grules.push("else_stmt", "ELSE")] = [](ParserContext& context)
    {
        checkControlStructureBlock(context.currentBlock);

        context.currentBlock.nestingLevel = context.currentNestingLevel;

        context.stack.push_back(std::make_any<BlockContent*>(new ElseStmt()));
    };

    m_semanticActionMap[ m_grules.push("endif_stmt", "ENDIF")] = [](ParserContext& context)
    {
        checkControlStructureBlock(context.currentBlock);

        context.currentBlock.nestingLevel = context.currentNestingLevel;
        context.currentNestingLevel--;

        context.stack.push_back(std::make_any<BlockContent*>(new EndIfStmt()));
    };

    using def_t = std::tuple<std::string, std::optional<Value>, std::optional<std::vector<s840d_int_t>>>;

    m_semanticActionMap[ m_grules.push("def_stmt", "DEF type def_list")] = [](ParserContext& context)
    {
        auto defs {std::any_cast<std::vector<def_t>>(context.stack.back())};
        context.stack.pop_back();
        auto typeStr {std::any_cast<std::string>(context.stack.back())};
        context.stack.pop_back();
        to_upper(typeStr);
        auto valueType {valueTypeFromString(typeStr)};
        if (valueType.has_value())
        {
            std::vector<DefStmt::Def> stmtDefs;
            std::vector<DefStmt::ArrayDef> stmtArrayDefs;
            for (auto& def : defs)
            {
                if (std::get<2>(def).has_value())
                {
                    stmtArrayDefs.emplace_back(
                        DefStmt::ArrayDef{std::get<0>(def), std::get<2>(def).value()});
                }
                else
                {
                    stmtDefs.emplace_back(
                        DefStmt::Def{std::get<0>(def),
                                     std::get<1>(def).value_or(createDefaultValue(valueType.value()))});
                }
            }

            context.stack.push_back(std::make_any<BlockContent*>(
                new DefStmt(std::move(stmtDefs), std::move(stmtArrayDefs), valueType.value())));
        }
        else
            throw std::runtime_error{"can not handle type " + typeStr};
    };
    m_semanticActionMap[ m_grules.push("type", "TYPE_STRING '[' INTEGER ']'")] = [](ParserContext& context)
    {
        context.stack.push_back(context.token(0).str());
    };
    m_semanticActionMap[ m_grules.push("type", "TYPE_OTHER")] = [](ParserContext& context)
    {
        context.stack.push_back(context.token(0).str());
    };

    m_semanticActionMap[ m_grules.push("def_list", "def_list ',' def")] = [](ParserContext& context)
    {
        auto def {std::any_cast<def_t>(context.stack.back())};
        context.stack.pop_back();
        auto& defs {std::any_cast<std::vector<def_t>&>(context.stack.back())};
        defs.emplace_back(std::move(def));
    };
    m_semanticActionMap[ m_grules.push("def_list", "def")] = [](ParserContext& context)
    {
        auto def {std::any_cast<def_t>(context.stack.back())};
        context.stack.pop_back();
        std::vector<def_t> defs;
        defs.emplace_back(std::move(def));
        context.stack.push_back(std::make_any<std::vector<def_t>>((std::move(defs))));
    };
    m_semanticActionMap[ m_grules.push("def", "IDENTIFIER")] = [](ParserContext& context)
    {
        auto idStr {context.token(0).str()};
        def_t def {idStr, std::nullopt, std::nullopt};
        context.stack.push_back(def);
    };
    m_semanticActionMap[ m_grules.push("def", "IDENTIFIER '[' INTEGER ']'")] = [](ParserContext& context)
    {
        auto idStr {context.token(0).str()};
        int i;
        try
        {
            i = stoi(context.token(2).str());
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12430};//specified index is invalid
        }
        def_t def {idStr, std::nullopt, std::vector<s840d_int_t>{i}};
        context.stack.push_back(def);
    };
    m_semanticActionMap[ m_grules.push("def", "IDENTIFIER '[' INTEGER ',' INTEGER ']'")] = [](ParserContext& context)
    {
        auto idStr {context.token(0).str()};
        int i, j;
        try
        {
            i = stoi(context.token(2).str());
            j = stoi(context.token(4).str());
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12430};//specified index is invalid
        }
        def_t def {idStr, std::nullopt, std::vector<s840d_int_t>{i, j}};
        context.stack.push_back(def);
    };
    m_semanticActionMap[ m_grules.push("def", "IDENTIFIER '[' INTEGER ',' INTEGER ',' INTEGER ']'")] = [](ParserContext& context)
    {
        auto idStr {context.token(0).str()};
        int i, j, k;
        try
        {
            i = stoi(context.token(2).str());
            j = stoi(context.token(4).str());
            k = stoi(context.token(6).str());
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12430};//specified index is invalid
        }
        def_t def {idStr, std::nullopt, std::vector<s840d_int_t>{i, j, k}};
        context.stack.push_back(def);
    };
    m_semanticActionMap[ m_grules.push("def", "IDENTIFIER '=' literal")] = [](ParserContext& context)
    {
        auto value {std::any_cast<Value>(context.stack.back())};
        context.stack.pop_back();
        auto idStr {context.token(0).str()};
        def_t def {idStr, value, std::nullopt};
        context.stack.push_back(def);
    };

    parsertl::generator::build(m_grules, m_gsm);
    m_grules.terminals(symbols_);
    m_grules.non_terminals(symbols_);

    // lexer setup
    addLexerRules(tokensLeftAssoc, m_grules, m_lrules);
    addLexerRules(tokens, m_grules, m_lrules);
    m_lrules.push("\\s+", lexertl::rules::skip());
    lexertl::generator::build(m_lrules, m_lsm);
    //m_lsm.minimise();
    //lexertl::debug::dump(m_lsm, std::cout);
    //parsertl::debug::dump(m_grules, std::cout);
}

void Parser::reset() noexcept
{
    nestingLevel = 0;
}


NCProgramBlock Parser::parse(const std::string& block)
{
    const auto commentPos {findCommentStartPos<std::string>(block.begin(), block.end())};

    if (0) {
        lexertl::citerator iter {block.c_str(), block.c_str() + commentPos, m_lsm};
        lexertl::citerator end;
        for (; iter != end; ++iter)
        {
            std::cout << "Id: " << iter->id << ", Token: '" <<
                iter->str() << "'\n";
        }
    }

    NCProgramBlock currentBlock;

    const auto start {block.c_str()};
    const auto end {start + commentPos};
    auto p = start;

    // read skip level, if any
    auto [skipLevelOpt, skipLevelEnd] {readSkipLevel(p, end)};
    if (skipLevelOpt.has_value())
    {
        currentBlock.skipLevel = skipLevelOpt.value();
        if (currentBlock.skipLevel >= 10)
            throw S840D_Alarm{14060}; //invalid skip level with differential block skip
    }
    p = skipLevelEnd;

    // read block number, if any
    auto [blockNumberOpt, blockNumberEnd] {readBlockNumber(p, end)};
    if (blockNumberOpt.has_value())
    {
        currentBlock.blockNumber = blockNumberOpt.value();
        if (currentBlock.blockNumber.m_number.size() > 30)
            throw S840D_Alarm{12420}; //identifier too long
    }
    p = blockNumberEnd;

    // read block number, if any
    auto [labelOpt, labelEnd] {readLabel(p, end)};
    if (labelOpt.has_value())
        currentBlock.label = labelOpt.value();
    p = labelEnd;

    lexertl::citerator iter {p, end, m_lsm};
    parsertl::match_results results {iter->id, m_gsm};
    m_productions.clear();

    ParserContext context {currentBlock, nestingLevel, m_stack, m_gsm, results, m_productions};
    context.currentBlock.blockContent.reserve(10);

    do
    {
        if (results.entry.action == parsertl::reduce)
        {
            auto action = m_semanticActionMap.find(results.entry.param);
            if (action != m_semanticActionMap.end())
            {
                // call semantic action function
                try
                {
                    action->second(context);
                }
                catch (const S840D_Alarm& alarm)
                {
                    // TODO cleanup
                    throw alarm;
                }
            }
if (0) {
            auto &pair_ = m_gsm._rules[results.entry.param];

            std::cout << "reduce by " << symbols_[pair_.first] << " ->";

            if (pair_.second.empty())
            {
                std::cout << " %empty";
            }
            else
            {
                for (auto iter_ = pair_.second.cbegin(),
                          end_ = pair_.second.cend(); iter_ != end_; ++iter_)
                {
                    std::cout << ' ' << symbols_[*iter_];
                }
            }

            std::cout << '\n';
}
        }
        else if (results.entry.action == parsertl::error)
        {
            // TODO cleanup
            throw S840D_Alarm{12080};//Syntax error
        }
        else if (results.entry.action == parsertl::shift)
        {
            if (0) {
                std::cout << "shift " << results.entry.param << '\n';
            }
        }
        else if (results.entry.action == parsertl::go_to)
        {
            if (0) {
                std::cout << "goto " << results.entry.param << '\n';
            }
        }

        parsertl::lookup(m_gsm, iter, results, m_productions);
    }
    while (results.entry.action != parsertl::accept);
    if (0) {
        std::cout << "accept\n";
    }

    return currentBlock;
}

const char* Parser::skipWS(const char* start, const char* end) const noexcept
{
    while (start != end && std::isspace(static_cast<unsigned char>(*start)))
        start++;

    return start;
}

std::pair<std::optional<int>, const char*> Parser::readSkipLevel(const char* start, const char* end) const
{
    auto p = start;
    p = skipWS(p, end);
    if (p == end || *p != '/')
        return {std::nullopt, p};
    p++;
    p = skipWS(p, end);
    auto digitStart = p;
    while (p != end && std::isdigit(static_cast<unsigned char>(*p)))
        p++;
    if (p != digitStart)
    {
        try
        {
            return {std::stoi(std::string {digitStart, p}), p};
        }
        catch (std::out_of_range&)
        {
            throw S840D_Alarm{12160}; //value lies beyond the value range
        }
    }
    return {0, p};
}

std::pair<std::optional<BlockNumber>, const char*> Parser::readBlockNumber(const char* start, const char* end) const
{
    auto p = start;
    p = skipWS(p, end);
    if (p == end || (*p != ':' && *p != 'N' && *p != 'n'))
        return {std::nullopt, p};
    auto type = *p == ':' ? BlockNumber::Main : BlockNumber::Regular;
    p++;
    p = skipWS(p, end);
    auto digitStart = p;
    while (p != end && std::isdigit(static_cast<unsigned char>(*p)))
        p++;
    if (p != digitStart)
        return {BlockNumber {std::string{digitStart, p}, type}, p};

    throw S840D_Alarm{12080};//Syntax error
}

std::pair<std::optional<std::string>, const char*> Parser::readLabel(const char* start, const char* end) const
{
    auto isIdStartChar = [](char c)
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '_';
    };
    auto isIdChar = [](char c)
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') ||
               c == '_';
    };

    auto p = start;
    p = skipWS(p, end);
    if (p == end || (end - p) < 3)
        return {std::nullopt, p};
    auto idStart = p;
    if (isIdStartChar(*p) &&
        isIdStartChar(*(p + 1)))
    {
        p += 2;
        while (p != end && isIdChar(*p))
            p++;
        if (*p == ':')
            return {std::string{idStart, p}, p + 1};
    }
    return {std::nullopt, idStart};
}
