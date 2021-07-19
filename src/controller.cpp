#include "controller.h"
#include "motion.h"
#include "value.h"
#include "util.h"
#include "scopedtimer.h"
#include "s840d_alarm.h"

#include <glm/vec4.hpp>

#include <QString>


inline static bool equalsIgnoreCase(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}

inline static bool equalsIgnoreCase(const std::string& a, const char* b)
{
    return std::equal(a.begin(), a.end(),
                      b, b+strlen(b),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}

Controller::Controller() noexcept
{
    initVariables();
}

void Controller::setListener(ControllerListener* listener) noexcept
{
    m_listener = listener;
}

void Controller::reset() noexcept
{
    m_sourceBlocks.clear();
    m_variables.clear();
    initVariables();
    m_defAllowed = true;
}

void Controller::addLine(const QString& line)
{
    m_sourceBlocks.emplace_back(line.toStdString());
}

void Controller::addLine(const std::string& line)
{
    m_sourceBlocks.push_back(line);
}

void Controller::run()
{
    {ScopedTimer t{"parsing"};
    // parse
    m_parser.reset();
    m_parsedBlocks.clear();
    m_parsedBlocks.reserve(m_sourceBlocks.size());
    for (auto& source : m_sourceBlocks)
    {
        try
        {
            m_parsedBlocks.push_back(m_parser.parse(source));
        }
        catch (const S840D_Alarm& alarm)
        {
            std::cout << "Alarm: " << alarm.getAlarmCode() << std::endl;
            break;
        }
    }
    }//ScopedTimer

    {ScopedTimer t{"evaluation"};

    gcodeResetValues();
    m_currentPointWCS = m_firstPoint;
    m_currentPointMCS = m_firstPoint;
    m_actFrame = Frame{};

    if (m_listener)
        m_listener->startPoint(m_currentPointWCS);

    size_t jumpCount{0};
    for (m_currentBlock = 0;
         m_currentBlock < m_parsedBlocks.size();
         )
    {
        if (m_listener)
            m_listener->blockChange(m_currentBlock);

        m_nextBlock = UNSET;

        NCProgramBlock& block {m_parsedBlocks[m_currentBlock]};
        try
        {
            evaluateBlock(block);
        }
        catch (const S840D_Alarm& alarm)
        {
            std::cout << "Alarm: " << alarm.getAlarmCode() << std::endl;
            break;
        }
        catch (const std::exception& e)
        {
            std::cout << "evaluation failed: exception " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cout << "evaluation failed: unknown error" << std::endl;
        }
        if (m_nextBlock == END_OF_PROGRAM)
            break;

        if (m_nextBlock != UNSET)
        {
            m_currentBlock = m_nextBlock;
            jumpCount++;
            // infinite loop protection
            if (jumpCount > m_maxJumpCount)
                break; // TODO warn user
        }
        else
            m_currentBlock++;
    }
    if (m_listener)
        m_listener->endOfProgram();

    }//ScopedTimer
}

bool Controller::handleGCodeGroup1(GCommands& gCommands, int gcode)
{
    // FIXME G[1]=... has no effect
    g_group_1 gGroupCode = g_group_1::UNDEF;
#define GCODE(NUMBER) case NUMBER: gGroupCode = g_group_1::G##NUMBER; break;
    switch (gcode)
    {
        GCODE(0)
        GCODE(1)
        GCODE(2)
        GCODE(3)
        GCODE(33)
        GCODE(331)
        GCODE(332)
        GCODE(34)
        GCODE(35)
        GCODE(335)
        GCODE(336)
    }
#undef GCODE

    if (gGroupCode != g_group_1::UNDEF)
    {
        gCommands.group1 = gGroupCode;
        return true;
    }
    return false;
}

bool Controller::handleGCodeGroup8(GCommands& gCommands, int gcode)
{
    uint8_t gGroupCode {0};
    if (gcode == 500)
    {
        gGroupCode = 1;
    }
    else if (gcode >= 54 && gcode <= 57)
    {
        gGroupCode = gcode - 52;
    }
    else if (gcode >= 54 && gcode <= 57)
    {
        gGroupCode = gcode - 52;
    }
    else if (gcode >= 505 && gcode <= 599)
    {
        gGroupCode = gcode - 499;
    }
    if (gGroupCode != 0)
    {
        gCommands.group8 = gGroupCode;
        return true;
    }
    return false;
}

bool Controller::handleGCodeGroup6(GCommands& gCommands, int gcode)
{
    g_group_6 gGroupCode = g_group_6::UNDEF;
#define GCODE(NUMBER) case NUMBER: gGroupCode = g_group_6::G##NUMBER; break;
    switch (gcode)
    {
        GCODE(17)
        GCODE(18)
        GCODE(19)
    }
#undef GCODE
    if (gGroupCode != g_group_6::UNDEF)
    {
        gCommands.group6 = gGroupCode;
        return true;
    }
    return false;
}

bool Controller::handleGCodeGroup7(GCommands& gCommands, int gcode)
{
    g_group_7 gGroupCode = g_group_7::UNDEF;
#define GCODE(NUMBER) case NUMBER: gGroupCode = g_group_7::G##NUMBER; break;
    switch (gcode)
    {
        GCODE(40)
        GCODE(41)
        GCODE(42)
    }
#undef GCODE
    if (gGroupCode != g_group_7::UNDEF)
    {
        gCommands.group7 = gGroupCode;
        return true;
    }
    return false;
}

bool Controller::handleGCodeGroup9(GCommands& gCommands, int gcode)
{
    g_group_9 gGroupCode = g_group_9::UNDEF;
#define GCODE(NUMBER) case NUMBER: gGroupCode = g_group_9::G##NUMBER; break;
    switch (gcode)
    {
        GCODE(53)
        GCODE(153)
    }
#undef GCODE
    if (gGroupCode != g_group_9::UNDEF)
    {
        gCommands.group9 = gGroupCode;
        return true;
    }
    return false;
}

bool Controller::handleGCodeGroup14(GCommands& gCommands, int gcode)
{
    g_group_14 gGroupCode = g_group_14::UNDEF;
#define GCODE(NUMBER) case NUMBER: gGroupCode = g_group_14::G##NUMBER; break;
    switch (gcode)
    {
        GCODE(90)
        GCODE(91)
    }
#undef GCODE
    if (gGroupCode != g_group_14::UNDEF)
    {
        gCommands.group14 = gGroupCode;
        return true;
    }
    return false;
}

bool Controller::handleGCodeGroup15(GCommands& gCommands, int gcode)
{
    g_group_15 gGroupCode = g_group_15::UNDEF;
#define GCODE(NUMBER) case NUMBER: gGroupCode = g_group_15::G##NUMBER; break;
    switch (gcode)
    {
        GCODE(93)
        GCODE(931)
        GCODE(94)
        case 942:
            handleGCodeGroup15(gCommands, getSpindleSpeedType(gCommands.group15) == RPM ? 94 : 961);
            break;
        GCODE(95)
        GCODE(96)
        GCODE(961)
        case 962:
            handleGCodeGroup15(gCommands, getFeedType(gCommands.group15) == PerMinute ? 961 : 96);
            break;
        GCODE(97)
        GCODE(971)
        case 972:
            handleGCodeGroup15(gCommands, getFeedType(gCommands.group15) == PerMinute ? 971 : 97);
            break;
        GCODE(973)
    }
#undef GCODE
    if (gGroupCode != g_group_15::UNDEF)
    {
        gCommands.group15 = gGroupCode;
        return true;
    }
    return false;
}

void Controller::visit(AddressAssign& addressAssign)
{
    // TODO check if non-default addressAssign.m_coordType is allowed

    //std::cout << __PRETTY_FUNCTION__ << ": address " << addressAssign.m_address << ", value " << convertValue(addressAssign.m_expr->evaluate(m_variables)) << std::endl;
    if (equalsIgnoreCase(addressAssign.m_address, m_axisConfig.getGeoAxisRef<1>()))
    {
        if (m_currentBlockState.xyz.x)
            throw S840D_Alarm{16420};

        m_currentBlockState.xyz.x = {assignCastReal(addressAssign.m_expr->evaluate(m_variables)), addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, m_axisConfig.getGeoAxisRef<2>()))
    {
        if (m_currentBlockState.xyz.y)
            throw S840D_Alarm{16420};

        m_currentBlockState.xyz.y = {assignCastReal(addressAssign.m_expr->evaluate(m_variables)), addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, m_axisConfig.getGeoAxisRef<3>()))
    {
        if (m_currentBlockState.xyz.z)
            throw S840D_Alarm{16420};

        m_currentBlockState.xyz.z = {assignCastReal(addressAssign.m_expr->evaluate(m_variables)), addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "I"))
    {
        if (m_currentBlockState.ijk.x)
            throw S840D_Alarm{16420};

        m_currentBlockState.ijk.x = {assignCastReal(addressAssign.m_expr->evaluate(m_variables)), addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "J"))
    {
        if (m_currentBlockState.ijk.y)
            throw S840D_Alarm{16420};

        m_currentBlockState.ijk.y = {assignCastReal(addressAssign.m_expr->evaluate(m_variables)), addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "K"))
    {
        if (m_currentBlockState.ijk.z)
            throw S840D_Alarm{16420};

        m_currentBlockState.ijk.z = {assignCastReal(addressAssign.m_expr->evaluate(m_variables)), addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "F"))
    {
        auto value = assignCastReal(addressAssign.m_expr->evaluate(m_variables));
        if (value <= 0.0)
            throw S840D_Alarm{14800};

        if (m_currentBlockState.realAddr.find("F") != m_currentBlockState.realAddr.end())
            throw S840D_Alarm{12010};

        m_currentBlockState.realAddr["F"] = value;
        m_feed = value;
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "G"))
    {
        auto gcode = assignCastInt(addressAssign.m_expr->evaluate(m_variables));
        bool handled =
                handleGCodeGroup1(m_currentBlockState.gCommands, gcode) ||
                handleGCodeGroup6(m_currentBlockState.gCommands, gcode) ||
                handleGCodeGroup7(m_currentBlockState.gCommands, gcode) ||
                handleGCodeGroup8(m_currentBlockState.gCommands, gcode) ||
                handleGCodeGroup9(m_currentBlockState.gCommands, gcode) ||
                handleGCodeGroup14(m_currentBlockState.gCommands, gcode) ||
                handleGCodeGroup15(m_currentBlockState.gCommands, gcode);
        if (!handled)
        {
            // TODO
        }
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "M"))
    {
        auto mcode = assignCastInt(addressAssign.m_expr->evaluate(m_variables));
        switch (mcode)
        {
        case 2:
        case 17:
        case 30:
            m_nextBlock = END_OF_PROGRAM;
            break;
        }
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "I1") ||
             equalsIgnoreCase(addressAssign.m_address, "J1") ||
             equalsIgnoreCase(addressAssign.m_address, "K1"))
    {
        auto value {assignCastReal(addressAssign.m_expr->evaluate(m_variables))};
        m_currentBlockState.coordAddr[addressAssign.m_address] = {value, addressAssign.m_coordType};
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "CR"))
    {
        auto value {assignCastReal(addressAssign.m_expr->evaluate(m_variables))};
        m_currentBlockState.realAddr[addressAssign.m_address] = value;
    }
    else if (equalsIgnoreCase(addressAssign.m_address, "TURN"))
    {
        auto value {assignCastInt(addressAssign.m_expr->evaluate(m_variables))};
        m_currentBlockState.intAddr[addressAssign.m_address] = value;
    }
}

void Controller::visit(LValueAssign& lvalueAssign)
{
    lvalueAssign.m_lvalueExpr->setValue(
        lvalueAssign.m_expr->evaluate(m_variables),
        m_variables);
}

void Controller::visit(ExtAddressAssign& extAddressAssign)
{
    if (equalsIgnoreCase(extAddressAssign.m_address, "G"))
    {
        int gGroup = assignCastInt(extAddressAssign.m_ext->evaluate(m_variables));
        if (gGroup >= 2 && gGroup <= 5)
            throw S840D_Alarm{12470};

        Value v = extAddressAssign.m_expr->evaluate(m_variables);
        int32_t intValue {assignCastInt(v)};
        if (intValue < 0 || intValue > std::numeric_limits<uint8_t>::max())
            throw S840D_Alarm{12475}; // invalid G function number programmed
        auto err {m_currentBlockState.gCommands.set(gGroup, static_cast<uint8_t>(intValue))};
        switch (err)
        {
        case GCommands::INVALID_INDEX:
            throw S840D_Alarm{12470}; // G function is unknown
        case GCommands::INVALID_VALUE:
            throw S840D_Alarm{12475}; // invalid G function number programmed
        default:
            ;// OK
        }
        m_variables.setArray1Value("$P_GG", gGroup, v);
    }
}

void Controller::visit(GCommand& func)
{
    auto group3 = [](GCommands& gCommands, g_group_3 gGroupCode)
    {
        if (gCommands.group3 == g_group_3::UNDEF)
            gCommands.group3 = gGroupCode;
        else
            throw S840D_Alarm{12070};
    };

#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: group3(m_currentBlockState.gCommands, g_group_3::COMMAND_NAME); break;
    switch (func.m_type)
    {
        COMMAND(ROT)
        COMMAND(AROT)
        COMMAND(TRANS)
        COMMAND(ATRANS)
        COMMAND(SCALE)
        COMMAND(ASCALE)
        COMMAND(MIRROR)
        COMMAND(AMIRROR)
        COMMAND(ROTS)
        COMMAND(AROTS)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group1 = g_group_1::COMMAND_NAME; break;
        COMMAND(CIP)
        COMMAND(ASPLINE)
        COMMAND(BSPLINE)
        COMMAND(CSPLINE)
        COMMAND(CT)
        COMMAND(POLY)
        COMMAND(INVCW)
        COMMAND(INVCCW)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group4 = g_group_4::COMMAND_NAME; break;
        COMMAND(STARTFIFO)
        COMMAND(STOPFIFO)
        COMMAND(FIFOCTRL)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group9 = g_group_9::COMMAND_NAME; break;
        COMMAND(SUPA)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group16 = g_group_16::COMMAND_NAME; break;
        COMMAND(CFC)
        COMMAND(CFTCP)
        COMMAND(CFIN)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group17 = g_group_17::COMMAND_NAME; break;
        COMMAND(NORM)
        COMMAND(KONT)
        COMMAND(KONTT)
        COMMAND(KONTC)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group24 = g_group_24::COMMAND_NAME; break;
        COMMAND(FFWOF)
        COMMAND(FFWON)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: m_currentBlockState.gCommands.group29 = g_group_29::COMMAND_NAME; break;
        COMMAND(DIAMOF)
        COMMAND(DIAMON)
        COMMAND(DIAM90)
        COMMAND(DIAMCYCOF)
#undef COMMAND
#define COMMAND(COMMAND_NAME) case GCommand::COMMAND_NAME: break;
        COMMAND(FNORM)
        COMMAND(FLIN)
        COMMAND(FCUB)
    }
#undef COMMAND
}

void Controller::visit(GotoStmt& gotoStmt)
{
    const Value target {gotoStmt.m_expr->evaluate(m_variables)};
    if (getValueType(target) != ValueType::STRING)
        throw S840D_Alarm{12150}; //operation not compatible with data type

    const std::string targetStr {std::get<s840d_string_t>(target)};
    const bool isBlockNum {isdigit(targetStr[0]) != 0};

    std::function<bool(NCProgramBlock&)> blockNumberCondition = [&targetStr](NCProgramBlock& block)
    {
        return block.blockNumber.m_number == targetStr;
    };
    std::function<bool(NCProgramBlock&)> labelCondition = [&targetStr](NCProgramBlock& block)
    {
        return block.label == targetStr;
    };
    auto searchCondition = isBlockNum ? blockNumberCondition : labelCondition;

    bool alarm {true};
    std::optional<size_t> index {std::nullopt};
    switch (gotoStmt.m_type)
    {
    case GotoStmt::GOTOB:
        index = blockSearchBack(searchCondition);
        break;
    case GotoStmt::GOTOF:
        index = blockSearchFwd(searchCondition);
        break;
    case GotoStmt::GOTOC:
        alarm = false;
        [[fallthrough]];
    case GotoStmt::GOTO:
        index = blockSearchFwdThenBack(searchCondition);
        break;
    default:
        throw std::runtime_error{"unreachable"};
    }

    if (index.has_value())
        m_nextBlock = index.value();
    else if (alarm)
        throw S840D_Alarm{14080};//destination not found
}

void Controller::visit(ConditionalGotoStmt& gotoStmt)
{
    ConditionalGotoStmt* stmt = &gotoStmt;
    do
    {
        Value condition {stmt->m_conditionExpr->evaluate(m_variables)};
        if (std::get<s840d_bool_t>(condition)) //TODO convert to bool?
        {
            stmt->m_gotoStmt->accept(*this);
            break;
        }
    }
    while ((stmt = stmt->m_next.get()) != nullptr);
}

void Controller::visit(ForStmt& forStmt)
{
    if (m_endforJump)
    {
        m_endforJump = false;

        // increment variable (only after first iteration)
        LiteralExpr oneExpr {1};
        Value incremented {BinaryOpExpr::evaluate(forStmt.m_assignment->m_lvalueExpr.get(),
                                                  &oneExpr,
                                                  BinaryOpExpr::ADD,
                                                  m_variables)};
        forStmt.m_assignment->m_lvalueExpr->setValue(incremented, m_variables);
    }
    else
    {
        // Assign initial value to the loop counter (only before first iteration)
        forStmt.m_assignment->accept(*this);
    }

    // Evaluate loop condition
    Value condition {BinaryOpExpr::evaluate(forStmt.m_assignment->m_lvalueExpr.get(),
                                            forStmt.m_expr.get(),
                                            BinaryOpExpr::LESS_OR_EQUAL,
                                            m_variables)};

    if (!std::get<s840d_bool_t>(condition))
    {
        int level = m_parsedBlocks[m_currentBlock].nestingLevel;
        auto searchCondition = [=](NCProgramBlock& block)
        {
            return block.blockContent.size() == 1 &&
                   dynamic_cast<EndForStmt*>(block.blockContent[0]) &&
                   block.nestingLevel == level;
        };

        if (auto index {blockSearchFwd(searchCondition)})
            m_nextBlock = index.value() + 1;
        else
            throw S840D_Alarm{12640}; //invalid nesting of control structures
    }
}

void Controller::visit(EndForStmt& /*endForStmt*/)
{
    int level = m_parsedBlocks[m_currentBlock].nestingLevel;
    auto searchCondition = [=](NCProgramBlock& block)
    {
        return block.blockContent.size() == 1 &&
               dynamic_cast<ForStmt*>(block.blockContent[0]) &&
               block.nestingLevel == level;
    };

    if (auto index {blockSearchBack(searchCondition)})
    {
        m_nextBlock = index.value();
        m_endforJump = true;
    }
    else
        throw S840D_Alarm{12640}; //invalid nesting of control structures
}

void Controller::visit(IfStmt& ifStmt)
{
    Value condition {ifStmt.m_expr->evaluate(m_variables)};
    if (!std::get<s840d_bool_t>(condition))
    {
        int level = m_parsedBlocks[m_currentBlock].nestingLevel;
        auto searchCondition = [=](NCProgramBlock& block)
        {
            return block.blockContent.size() == 1 &&
                   (dynamic_cast<ElseStmt*>(block.blockContent[0]) ||
                    dynamic_cast<EndIfStmt*>(block.blockContent[0])) &&
                   block.nestingLevel == level;
        };
        if (auto index {blockSearchFwd(searchCondition)})
        {
            m_nextBlock = index.value() + 1;
            m_endforJump = true;
        }
        else
            throw S840D_Alarm{12640}; //invalid nesting of control structures
    }
}

void Controller::visit(ElseStmt& /*elseStmt*/)
{
    int level = m_parsedBlocks[m_currentBlock].nestingLevel;
    auto searchCondition = [=](NCProgramBlock& block)
    {
        return block.blockContent.size() == 1 &&
               dynamic_cast<EndIfStmt*>(block.blockContent[0]) &&
               block.nestingLevel == level;
    };
    if (auto index {blockSearchFwd(searchCondition)})
    {
        m_nextBlock = index.value();
        m_endforJump = true;
    }
    else
        throw S840D_Alarm{12640}; //invalid nesting of control structures
}

void Controller::visit(EndIfStmt& /*endIfStmt*/)
{
    // ENDIF serves the only purpose - it is a branch target
}

void Controller::visit(DefStmt& defStmt)
{
    auto resultHandler = [](Variables::DefineResult result) {
        switch (result)
        {
        case Variables::DefineResult::Success:
            break;
        case Variables::DefineResult::AlreadyExists:
            throw S840D_Alarm{12170}; // name defined several times
        case Variables::DefineResult::InvalidArraySize:
        case Variables::DefineResult::InvalidDimensionCount:
            break; // TODO
        case Variables::DefineResult::OutOfMemory:
            throw S840D_Alarm{12380}; // maximum memory capacity reached
        case Variables::DefineResult::UnknownError:
            throw std::runtime_error{"def stmt: internal error"};
        }
    };
    for (auto& def : defStmt.m_defs)
    {
        auto result {m_variables.define(def.varName, assignCast(def.initValue, defStmt.m_type))};
        resultHandler(result);
    }
    for (auto& arrayDef : defStmt.m_arrayDefs)
    {
        auto result {m_variables.defineArray(arrayDef.varName, defStmt.m_type, arrayDef.arrayDimensions)};
        resultHandler(result);
    }
}

void Controller::initVariables()
{
    m_variables.defineArray("R", ValueType::REAL, std::vector<int>({100}));
    m_variables.defineArray("$P_GG", ValueType::INT, std::vector<int>({65}));
}

void Controller::copyDefinedModalGFunctions(const GCommands& from, GCommands& to)
{
    for (unsigned i {1}; i <= GCommands::size; i++)
    {
        uint8_t value;
        from.get(i, &value);
        if (value != 0)
            to.set(i, value);
    }

    to.group2 = g_group_2::UNDEF;
    to.group3 = g_group_3::UNDEF;
    to.group9 = g_group_9::UNDEF;
    to.group11 = g_group_11::UNDEF;
//    to.group62 = g_group_62::UNDEF;
}

void Controller::evaluateBlock(NCProgramBlock& block)
{
    m_currentBlockState = {};

    bool isDef {isDefSectionBlock(block)};
    if (m_defAllowed)
    {
        if (!isDef)
            m_defAllowed = false;
    }
    else
    {
        if (isDef)
            throw S840D_Alarm{14500}; // illegal DEF or PROC instruction in the part program
    }

    for (auto content : block.blockContent)
        content->accept(*this);

    int gGroup1 = m_currentBlockState.gCommands.group1 != g_group_1::UNDEF;
    int gGroup2 = m_currentBlockState.gCommands.group2 != g_group_2::UNDEF;
    int gGroup3 = m_currentBlockState.gCommands.group3 != g_group_3::UNDEF;
    int gFuncCount = gGroup1 + gGroup2 + gGroup3;
    if (gFuncCount > 1)
        throw S840D_Alarm{12070};//too many syntax-defining G-functions

    copyDefinedModalGFunctions(m_currentBlockState.gCommands, m_gCommands);

    if (m_currentBlockState.gCommands.group3 != g_group_3::UNDEF)
    {
        switch (m_currentBlockState.gCommands.group3)
        {
        case g_group_3::TRANS:
            m_actFrame.setTrans(m_currentBlockState.xyz);
            break;
        case g_group_3::ROT:
            m_actFrame.setRot(m_currentBlockState.xyz);
            break;
        case g_group_3::SCALE:
            break;
        case g_group_3::MIRROR:
            break;
        case g_group_3::ATRANS:
            m_actFrame.addTrans(m_currentBlockState.xyz);
            break;
        case g_group_3::AROT:
            m_actFrame.addRot(m_currentBlockState.xyz);
            break;
        case g_group_3::ASCALE:
            break;
        case g_group_3::AMIRROR:
            break;
        default:
            break;
        }
    }
    else if (m_currentBlockState.gCommands.group2 != g_group_2::UNDEF)
    {

    }
    else if (m_gCommands.group1 != g_group_1::UNDEF /*???*/)
    {
        if (((m_gCommands.group1 == g_group_1::G0 || m_gCommands.group1 == g_group_1::G1 || m_gCommands.group1 == g_group_1::CIP) &&
             m_currentBlockState.xyz.hasAnyValue()) ||
            ((m_gCommands.group1 == g_group_1::G2 || m_gCommands.group1 == g_group_1::G3) &&
             (m_currentBlockState.ijk.hasAnyValue() ||
              (m_currentBlockState.realAddr.count("CR") && m_currentBlockState.xyz.hasAnyValue()))))
        {
            bool isRapid = (m_gCommands.group1 == g_group_1::G0);
            if (!isRapid && m_feed == 0.0)
                throw S840D_Alarm{10860}; //feed rate not programmed

            glm::dmat4 actTransform {m_actFrame.toMat()};
            glm::dmat4 actTransformInv {glm::inverse(actTransform)};
            m_currentPointWCS = actTransformInv * glm::dvec4(m_currentPointMCS, 1.0);
            auto prevPointWCS {m_currentPointWCS};
            auto prevPointMCS {m_currentPointMCS};
            auto coordType {m_gCommands.group14 == g_group_14::G90 ? AddressAssign::AC : AddressAssign::IC};
            m_currentBlockState.xyz.set_dvec3(m_currentPointWCS, coordType);

            m_currentPointMCS = actTransform * glm::dvec4(m_currentPointWCS, 1.0);
            if (m_gCommands.group1 == g_group_1::G0 || m_gCommands.group1 == g_group_1::G1)
            {
                LinearMotion lm {m_currentPointMCS, isRapid ? 0 : m_feed};
                if (m_listener)
                    m_listener->linearMotion(lm);
            }
            else if (m_gCommands.group1 == g_group_1::G2 || m_gCommands.group1 == g_group_1::G3)
            {
                auto dir {m_gCommands.group1 == g_group_1::G2 ? DirectedArc2::clw : DirectedArc2::cclw};

                auto createCircularMotion = [this, &actTransform, &prevPointWCS](const std::optional<DirectedArc2>& arc2,
                                                                                 g_group_6 wp,
                                                                                 bool forceHelix = false)
                {
                    if (arc2.has_value())
                    {
                        unsigned turn {0};
                        auto it {m_currentBlockState.intAddr.find("TURN")};
                        bool turnAddr {it != m_currentBlockState.intAddr.end()};
                        if (turnAddr || forceHelix)
                        {
                            if (turnAddr)
                            {
                                if (it->second < 0)
                                    throw S840D_Alarm{14048}; //wrong number of revolutions in circle programming
                                turn = static_cast<unsigned>(it->second);
                            }

                            Helix helix {arc2.value(), actTransform * wpRot(wp), wpZ(prevPointWCS, wp), wpZ(m_currentPointWCS, wp), turn};
                            HelicalMotion hm {helix, m_feed};
                            if (m_listener)
                                m_listener->helicalMotion(hm);
                        }
                        else
                        {
                            DirectedArc3 arc3 {arc2.value(), actTransform * wpRot(wp), wpZ(m_currentPointWCS, wp)};
                            CircularMotion cm {arc3, m_feed};
                            if (m_listener)
                                m_listener->circularMotion(cm);
                        }
                    }
                    else
                    {
                        throw S840D_Alarm{14040};//error in end point of circle
                    }
                };

                g_group_6 wp;
                if (m_currentBlockState.xyz.count() == 2)
                {
                    if (m_currentBlockState.xyz.x && m_currentBlockState.xyz.y)
                        wp = g_group_6::G17;
                    else if (m_currentBlockState.xyz.x && m_currentBlockState.xyz.z)
                        wp = g_group_6::G18;
                    else
                        wp = g_group_6::G19;
                }
                else
                    wp = m_gCommands.group6;

                if (auto it = m_currentBlockState.realAddr.find("CR");
                    it != m_currentBlockState.realAddr.end())
                {
                    auto radius = it->second;
                    createCircularMotion(
                        DirectedArc2::create2PointsRadius(
                            wpXY(prevPointWCS, wp), wpXY(m_currentPointWCS, wp), radius, dir, m_arcTolerance),
                        wp, m_currentBlockState.xyz.count() == 3);
                }
                else
                {
                    glm::dvec3 centerPointWCS {prevPointWCS};
                    m_currentBlockState.ijk.set_dvec3(centerPointWCS, AddressAssign::IC);

                    createCircularMotion(
                        DirectedArc2::create2PointsCenter(
                            wpXY(centerPointWCS, wp),
                            wpXY(prevPointWCS, wp),
                            wpXY(m_currentPointWCS, wp),
                            dir, m_arcTolerance),
                        wp, m_currentBlockState.xyz.count() == 3);
                }
            }
            else if (m_gCommands.group1 == g_group_1::CIP)
            {
                glm::dvec3 intermediatePointWCS {prevPointWCS};
                if (auto it = m_currentBlockState.coordAddr.find("I1");
                    it != m_currentBlockState.coordAddr.end())
                {
                    it->second.setValue(intermediatePointWCS.x, coordType);
                }
                if (auto it = m_currentBlockState.coordAddr.find("J1");
                    it != m_currentBlockState.coordAddr.end())
                {
                    it->second.setValue(intermediatePointWCS.y, coordType);
                }
                if (auto it = m_currentBlockState.coordAddr.find("K1");
                    it != m_currentBlockState.coordAddr.end())
                {
                    it->second.setValue(intermediatePointWCS.z, coordType);
                }
                auto intermediatePointMCS {actTransform * glm::dvec4(intermediatePointWCS, 1.0)};

                auto arc3 {DirectedArc3::create3Points(prevPointMCS, intermediatePointMCS, m_currentPointMCS, 0/*TODO*/)};
                CircularMotion cm {arc3.value(), m_feed};
                if (m_listener)
                    m_listener->circularMotion(cm);
            }
        }
    }
}

bool Controller::isDefSectionBlock(const NCProgramBlock& block) const noexcept
{
    return block.blockContent.size() == 1 && dynamic_cast<DefStmt*>(block.blockContent.front());
}

void Controller::gcodeResetValues()
{
    // copied from MD20150, but here we start from index 1, not 0
    constexpr std::array md20150 {0, 2, 0, 0, 2, 0, 1, 1, 1, 0, 1, 0, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    for (unsigned index {1}; index < md20150.size(); index++)
    {
        m_gCommands.set(index, md20150[index]);
        m_variables.setArray1Value("$P_GG", (int)index, md20150[index]);
    }
}

std::optional<size_t> Controller::blockSearchFwd(std::function<bool (NCProgramBlock&)> condition)
{
    for (auto index {m_currentBlock + 1}; index < m_parsedBlocks.size(); index++)
        if (condition(m_parsedBlocks[index]))
            return index;

    return std::nullopt;
}

std::optional<size_t> Controller::blockSearchBack(std::function<bool (NCProgramBlock&)> condition)
{
    for (auto index {static_cast<int>(m_currentBlock - 1)}; index >= 0; index--)
        if (condition(m_parsedBlocks[index]))
            return index;

    return std::nullopt;
}

std::optional<size_t> Controller::blockSearchFwdThenBack(std::function<bool (NCProgramBlock&)> condition)
{
    if (auto index {blockSearchFwd(condition)})
        return index;
    return blockSearchBack(condition);
}

glm::dvec2 Controller::wpXY(const glm::dvec3& v, g_group_6 wp) const
{
    switch (wp)
    {
    case g_group_6::G17:
        return {v.x, v.y};
    case g_group_6::G18:
        return {v.z, v.x};
    case g_group_6::G19:
        return {v.y, v.z};
    default:
        throw std::runtime_error{"Illegal G-group 6 code"};
    }
}

double Controller::wpZ(const glm::dvec3& v, g_group_6 wp) const
{
    switch (wp)
    {
    case g_group_6::G17:
        return v.z;
    case g_group_6::G18:
        return v.y;
    case g_group_6::G19:
        return v.x;
    default:
        throw std::runtime_error{"Illegal G-group 6 code"};
    }
}

glm::dmat4 Controller::wpRot(g_group_6 wp) const
{
    switch (wp)
    {
    case g_group_6::G17:
        return glm::dmat3(1.0);
    case g_group_6::G18:
        return glm::dmat3(0.0, 0.0, 1.0,
                          1.0, 0.0, 0.0,
                          0.0, 1.0, 0.0);
    case g_group_6::G19:
        return glm::dmat3(0.0, 1.0, 0.0,
                          0.0, 0.0, 1.0,
                          1.0, 0.0, 0.0);
    default:
        throw std::runtime_error{"Illegal G-group 6 code"};
    }
}

Controller::SpindleSpeedType Controller::getSpindleSpeedType(g_group_15 gcode) const
{
    switch (gcode)
    {
    case g_group_15::G96:
    case g_group_15::G961:
        return SurfaceSpeed;
    default:
        return RPM;
    }
}

Controller::FeedType Controller::getFeedType(g_group_15 gcode) const
{
    switch (gcode)
    {
    case g_group_15::G93:
        return InvTime;
    case g_group_15::G931:
        return Time;
    case g_group_15::G95:
    case g_group_15::G96:
    case g_group_15::G97:
        return PerRevolution;
    default:
        return PerMinute;
    }
}

Controller::GCommands::ErrorCode Controller::GCommands::set(unsigned index, uint8_t value)
{
    if (index == 0 || index > GCommands::size)
        return INVALID_INDEX;
    if (value > maxValues[index - 1])
        return INVALID_VALUE;

    reinterpret_cast<uint8_t*>(this)[index - 1] = value;
    return OK;
}

Controller::GCommands::ErrorCode Controller::GCommands::get(unsigned index, uint8_t* value) const
{
    if (index == 0 || index > GCommands::size)
        return INVALID_INDEX;

    *value = reinterpret_cast<const uint8_t*>(this)[index - 1];
    return OK;
}
