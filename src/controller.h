#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "parser.h"
#include "util.h"
#include "motion.h"
#include "variables.h"
#include "ncprogramblock.h"
#include "ggroupenum.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <optional>
#include <vector>
#include <memory>
#include <functional>

class QString;

class ControllerListener
{
public:
    virtual ~ControllerListener() = default;
    virtual void startPoint(const glm::dvec3& point) = 0;
    virtual void blockChange(size_t blockNumber) = 0;
    virtual void linearMotion(const LinearMotion& linearMotion) = 0;
    virtual void circularMotion(const CircularMotion& circularMotion) = 0;
    virtual void helicalMotion(const HelicalMotion& helicalMotion) = 0;
    virtual void endOfProgram() = 0;
};

/**
 * Represents a S840D controller.
 */
class Controller : public BlockContentVisitor
{
public:
    Controller() noexcept;

    ~Controller() = default;
    Controller(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller& operator=(Controller&&) = delete;

    void setListener(ControllerListener* listener) noexcept;
    void addLine(const QString& line);
    void addLine(const std::string& line);
    void reset() noexcept;
    void run();

    // BlockContentVisitor interface
    void visit(AddressAssign& addressAssign) override;
    void visit(LValueAssign& lvalueAssign) override;
    void visit(ExtAddressAssign& extAddressAssign) override;
    void visit(GCommand& func) override;
    void visit(GotoStmt& gotoStmt) override;
    void visit(ConditionalGotoStmt& gotoStmt) override;
    void visit(ForStmt& forStmt) override;
    void visit(EndForStmt& /*endForStmt*/) override;
    void visit(IfStmt& ifStmt) override;
    void visit(ElseStmt& /*elseStmt*/) override;
    void visit(EndIfStmt& /*endIfStmt*/) override;
    void visit(DefStmt& defStmt) override;

private:
    struct GCommands
    {
        g_group_1  group1;
        g_group_2  group2;
        g_group_3  group3;
        g_group_4  group4;
        uint8_t    group5_dummy;
        g_group_6  group6;
        g_group_7  group7;
        uint8_t    group8;
        g_group_9  group9;
        g_group_10 group10;
        g_group_11 group11;
        g_group_12 group12;
        g_group_13 group13;
        g_group_14 group14;
        g_group_15 group15;
        g_group_16 group16;
        g_group_17 group17;
        g_group_18 group18;
        g_group_19 group19;
        g_group_20 group20;
        g_group_21 group21;
        g_group_22 group22;
        g_group_23 group23;
        g_group_24 group24;
        g_group_25 group25;
        g_group_26 group26;
        g_group_27 group27;
        g_group_28 group28;
        g_group_29 group29;
        g_group_30 group30;

        enum ErrorCode
        {
            OK,
            INVALID_VALUE,
            INVALID_INDEX
        };

        ErrorCode set(unsigned index, uint8_t value);
        ErrorCode get(unsigned index, uint8_t* value) const;

        static constexpr unsigned size {30};
        static constexpr std::array<uint8_t, size> maxValues
        {
            static_cast<uint8_t>(g_group_1::MAX),
            static_cast<uint8_t>(g_group_2::MAX),
            static_cast<uint8_t>(g_group_3::MAX),
            static_cast<uint8_t>(g_group_4::MAX),
            0,
            static_cast<uint8_t>(g_group_6::MAX),
            static_cast<uint8_t>(g_group_7::MAX),
            100,
            static_cast<uint8_t>(g_group_9::MAX),
            static_cast<uint8_t>(g_group_10::MAX),
            static_cast<uint8_t>(g_group_11::MAX),
            static_cast<uint8_t>(g_group_12::MAX),
            static_cast<uint8_t>(g_group_13::MAX),
            static_cast<uint8_t>(g_group_14::MAX),
            static_cast<uint8_t>(g_group_15::MAX),
            static_cast<uint8_t>(g_group_16::MAX),
            static_cast<uint8_t>(g_group_17::MAX),
            static_cast<uint8_t>(g_group_18::MAX),
            static_cast<uint8_t>(g_group_19::MAX),
            static_cast<uint8_t>(g_group_20::MAX),
            static_cast<uint8_t>(g_group_21::MAX),
            static_cast<uint8_t>(g_group_22::MAX),
            static_cast<uint8_t>(g_group_23::MAX),
            static_cast<uint8_t>(g_group_24::MAX),
            static_cast<uint8_t>(g_group_25::MAX),
            static_cast<uint8_t>(g_group_26::MAX),
            static_cast<uint8_t>(g_group_27::MAX),
            static_cast<uint8_t>(g_group_28::MAX),
            static_cast<uint8_t>(g_group_29::MAX),
            static_cast<uint8_t>(g_group_30::MAX)
        };
    };

    struct CoordValue
    {
        double m_value;
        AddressAssign::CoordType m_type;

        void setValue(double& value, AddressAssign::CoordType defaultCoordType) const
        {
            auto type {m_type != AddressAssign::DEFAULT ? m_type : defaultCoordType};
            if (type == AddressAssign::IC)
                value += m_value;
            else
                value = m_value;
        }
    };

    struct CoordVector
    {
        std::optional<CoordValue> x;
        std::optional<CoordValue> y;
        std::optional<CoordValue> z;

        bool hasAnyValue() const
        {
            return x.has_value() || y.has_value() || z.has_value();
        };
        glm::dvec3 to_dvec3() const
        {
            glm::dvec3 v {0.0};
            if (x) v.x = x->m_value;
            if (y) v.y = y->m_value;
            if (z) v.z = z->m_value;
            return v;
        }
        void set_dvec3(glm::dvec3& v, AddressAssign::CoordType defaultCoordType) const
        {
            if (x) x->setValue(v.x, defaultCoordType);
            if (y) y->setValue(v.y, defaultCoordType);
            if (z) z->setValue(v.z, defaultCoordType);
        }
        unsigned count() const
        {
            unsigned count {0};
            if (x) count++;
            if (y) count++;
            if (z) count++;
            return count;
        }
    };


    struct State
    {
        CoordVector xyz;
        CoordVector ijk;
        std::map<std::string, CoordValue> coordAddr;
        std::map<std::string, s840d_real_t> realAddr;
        std::map<std::string, s840d_int_t> intAddr;
        GCommands gCommands = {};
    };

    class Frame
    {
        glm::dmat4 mat {1.0};

    public:
        void addTrans(const CoordVector& trans)
        {
            mat = glm::translate(mat, trans.to_dvec3());
        }
        void setTrans(const CoordVector& trans)
        {
            mat = glm::dmat4(1.0);
            addTrans(trans);
        }

        void addRot(const CoordVector& rot)
        {
            if (rot.z)
                mat = glm::rotate(mat, glm::radians(rot.z->m_value), glm::dvec3(0.0, 0.0, 1.0));
            if (rot.y)
                mat = glm::rotate(mat, glm::radians(rot.y->m_value), glm::dvec3(0.0, 1.0, 0.0));
            if (rot.x)
                mat = glm::rotate(mat, glm::radians(rot.x->m_value), glm::dvec3(1.0, 0.0, 0.0));
        }
        void setRot(const CoordVector& rot)
        {
            mat = glm::dmat4(1.0);
            addRot(rot);
        }

        glm::dmat4 toMat()
        {
            return mat;
        }
    };

    class AxisConfiguration
    {
        std::array<std::string, 3> geoAxes{"X", "Y", "Z"};
        std::array<std::string, 3> circleAddr{"I", "J", "K"};

    public:
        template<std::size_t n>
        void setGeoAxis(const std::string& ax)
        {
            static_assert(n >= 1 && n <= 3);
            geoAxes[n-1] = ax;
        }
        template<std::size_t n>
        std::string getGeoAxis() const
        {
            static_assert(n >= 1 && n <= 3);
            return geoAxes[n-1];
        }
        template<std::size_t n>
        const std::string& getGeoAxisRef() const
        {
            static_assert(n >= 1 && n <= 3);
            return geoAxes[n-1];
        }
        template<std::size_t n>
        void setCircleAddress(const std::string& ax)
        {
            static_assert(n >= 1 && n <= 3);
            circleAddr[n-1] = ax;
        }
        template<std::size_t n>
        std::string getCircleAddress() const
        {
            static_assert(n >= 1 && n <= 3);
            return circleAddr[n-1];
        }
        template<std::size_t n>
        const std::string& getCircleAddressRef() const
        {
            static_assert(n >= 1 && n <= 3);
            return circleAddr[n-1];
        }
    };



    void initVariables();
    void evaluateBlock(NCProgramBlock& block);
    bool isDefSectionBlock(const NCProgramBlock& block) const noexcept;
    void gcodeResetValues();

    bool handleGCodeGroup1(GCommands& gCommands, int gcode);
    bool handleGCodeGroup6(GCommands& gCommands, int gcode);
    bool handleGCodeGroup7(GCommands& gCommands, int gcode);
    bool handleGCodeGroup8(GCommands& gCommands, int gcode);
    bool handleGCodeGroup9(GCommands& gCommands, int gcode);
    bool handleGCodeGroup14(GCommands& gCommands, int gcode);
    bool handleGCodeGroup15(GCommands& gCommands, int gcode);

    std::optional<size_t> blockSearchFwd(std::function<bool(NCProgramBlock&)> condition);
    std::optional<size_t> blockSearchBack(std::function<bool(NCProgramBlock&)> condition);
    std::optional<size_t> blockSearchFwdThenBack(std::function<bool(NCProgramBlock&)> condition);

    glm::dvec2 wpXY(const glm::dvec3& v, g_group_6 wp) const;
    double wpZ(const glm::dvec3& v, g_group_6 wp) const;
    glm::dmat4 wpRot(g_group_6 wp) const;

    static void copyDefinedModalGFunctions(const GCommands& from, GCommands& to);

    const size_t m_maxJumpCount {1000000};

    ControllerListener* m_listener {};

    AxisConfiguration m_axisConfig;
    Variables m_variables;
    std::vector<std::string> m_sourceBlocks;
    std::vector<NCProgramBlock> m_parsedBlocks;
    Parser m_parser;

    glm::dvec3 m_firstPoint {0.0};//for now
    glm::dvec3 m_currentPointWCS {m_firstPoint};
    glm::dvec3 m_currentPointMCS {m_firstPoint};

    enum FeedType
    {
        PerMinute,
        PerRevolution,
        InvTime, // 1/minute
        Time // seconds
    };
    enum SpindleSpeedType
    {
        RPM,
        SurfaceSpeed
    };

    SpindleSpeedType getSpindleSpeedType(g_group_15 gcode) const;
    FeedType getFeedType(g_group_15 gcode) const;

    // current settings
    double m_feed {0.0};
    double m_arcTolerance {0.015};

    bool m_defAllowed {true};

    GCommands m_gCommands;

    State m_currentBlockState;
    Frame m_actFrame;

    std::size_t m_currentBlock {0};
    std::size_t m_nextBlock {0};

    bool m_endforJump{false};

    const size_t UNSET {std::numeric_limits<std::size_t>::max()-1};
    const size_t END_OF_PROGRAM {std::numeric_limits<std::size_t>::max()-2};

};

#endif // CONTROLLER_H
