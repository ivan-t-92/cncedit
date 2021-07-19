#include <QtTest>

#include "geometry.h"
#include "controller.h"

#include <glm/gtc/epsilon.hpp>

struct TestMotionHandler : public ControllerListener
{
    glm::dvec3 m_point;
    double m_feed {};

    void startPoint(const glm::dvec3& point) override
    {
        m_point = point;
    }
    void blockChange(size_t /*blockNumber*/) override {};
    void linearMotion(const LinearMotion& linearMotion) override
    {
        m_point = linearMotion.getEndPoint();
        m_feed = linearMotion.getFeed();
    }
    void circularMotion(const CircularMotion& circularMotion) override
    {
        DirectedArc3Sampler s {circularMotion.getArc()};
        m_point = s.sample(1.0);
        m_feed = circularMotion.getFeed();
    }
    void helicalMotion(const HelicalMotion& helicalMotion) override
    {
        HelixSampler s {helicalMotion.getHelix()};
        m_point = s.sample(1.0);
        m_feed = helicalMotion.getFeed();
    }
    void endOfProgram() override {}
};

class test_case_1 : public QObject
{
    Q_OBJECT


public:
    test_case_1();
    ~test_case_1();

private slots:
    void test_case1();
    void arc2_create_2_points_center();
    void arc2_create_2_points_radius();
    void arc2_create_3_points();
    void arc2_sampling();

    void arc3_create_3_points();

    void helix_sampling();
};

test_case_1::test_case_1()
{

}

test_case_1::~test_case_1()
{

}

void test_case_1::test_case1()
{
    TestMotionHandler h;
    Controller c;
    c.setListener(&h);
    c.addLine(std::string("G0 X=(10+2*3)"));
    c.run();
    QVERIFY(h.m_point == glm::dvec3(16, 0, 0));
    QVERIFY(h.m_feed == 0);
}

void test_case_1::arc2_create_2_points_center()
{
    {
        auto arc = DirectedArc2::create2PointsCenter({0,0}, {1,0}, {0,1}, DirectedArc2::clw, 0);
        QVERIFY(arc.has_value());
    }
    {
        auto arc = DirectedArc2::create2PointsCenter({0,0}, {1.1,0}, {0,1}, DirectedArc2::clw, 0.09);
        QVERIFY(!arc.has_value());
    }
}

void test_case_1::arc2_create_2_points_radius()
{
    const double eps {1e-14};
    {
        auto arc = DirectedArc2::create2PointsRadius({5,5}, {10,10}, 5, DirectedArc2::clw, 0);
        QVERIFY(arc.has_value() && glm::all(glm::epsilonEqual(arc->center, glm::dvec2(10,5), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsRadius({5,5}, {10,10}, -5, DirectedArc2::clw, 0);
        QVERIFY(arc.has_value() && glm::all(glm::epsilonEqual(arc->center, glm::dvec2(5,10), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsRadius({5,5}, {10,10}, 5, DirectedArc2::cclw, 0);
        QVERIFY(arc.has_value() && glm::all(glm::epsilonEqual(arc->center, glm::dvec2(5,10), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsRadius({5,5}, {10,10}, -5, DirectedArc2::cclw, 0);
        QVERIFY(arc.has_value() && glm::all(glm::epsilonEqual(arc->center, glm::dvec2(10,5), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsRadius({5,5}, {5,10}, 2.49, DirectedArc2::clw, 0);
        QVERIFY(!arc.has_value());
    }
}

void test_case_1::arc2_create_3_points()
{
    const double eps {1e-14};
    {
        glm::dvec2 center(5,10);
        glm::dvec2 rad(3,0);
        auto arc = DirectedArc2::create3Points(center + glm::rotate(rad, 1.0),
                                               center + glm::rotate(rad, 3.0),
                                               center + glm::rotate(rad, 6.0), 0);
        QVERIFY(arc.has_value());
        QVERIFY(arc->dir == DirectedArc2::cclw);
        QVERIFY(glm::all(glm::epsilonEqual(arc->center, center, eps)));
    }
    {
        glm::dvec2 center(5,10);
        glm::dvec2 rad(3,0);
        auto arc = DirectedArc2::create3Points(center + glm::rotate(rad, -1.0),
                                               center + glm::rotate(rad, -3.0),
                                               center + glm::rotate(rad, -6.0), 0);
        QVERIFY(arc.has_value());
        QVERIFY(arc->dir == DirectedArc2::clw);
        QVERIFY(glm::all(glm::epsilonEqual(arc->center, center, eps)));
    }
}

void test_case_1::arc2_sampling()
{
    const double eps {1e-12};
    {
        auto arc = DirectedArc2::create2PointsCenter({0,0}, {1,0}, {-1,0}, DirectedArc2::cclw, 0);
        DirectedArc2Sampler s(*arc);
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.5), glm::dvec2(0,1), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.25), glm::dvec2(sqrt(2)*0.5), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.75), glm::dvec2(-sqrt(2)*0.5, sqrt(2)*0.5), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsCenter({0,0}, {1,0}, {0,-1}, DirectedArc2::cclw, 0);
        DirectedArc2Sampler s(*arc);
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(1.0/3.0), glm::dvec2(0,1), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(2.0/3.0), glm::dvec2(-1,0), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsCenter({0,0}, {1,0}, {1,0}, DirectedArc2::cclw, 0);
        DirectedArc2Sampler s(*arc);
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.5), glm::dvec2(-1,0), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.25), glm::dvec2(0,1), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsCenter({0,0}, {1,0}, {1,0}, DirectedArc2::clw, 0);
        DirectedArc2Sampler s(*arc);
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.5), glm::dvec2(-1,0), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.25), glm::dvec2(0,-1), eps)));
    }
    {
        auto arc = DirectedArc2::create2PointsCenter({5,2}, {10,2}, {5,7}, DirectedArc2::cclw, 0);
        DirectedArc2Sampler s(*arc);
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.5), glm::dvec2(5+5*sqrt(2)*0.5,2+5*sqrt(2)*0.5), eps)));
    }
    {
        auto r {20.0};
        auto h {r * std::cos(glm::radians(30.0))};
        auto arc = DirectedArc2::create2PointsCenter({r*0.5,-h}, {0,0}, {r,0}, DirectedArc2::cclw, 0);
        DirectedArc2Sampler s(*arc);
        auto v {s.sample(0.5)};
        QVERIFY(glm::all(glm::epsilonEqual(v, glm::dvec2(r*0.5,-(r+h)), eps)));
    }
}

void test_case_1::arc3_create_3_points()
{
    const double eps {1e-10};
    {
        auto arc = DirectedArc3::create3Points({5,10, 0}, {0,0,20}, {25, 1, 0}, 0);
        DirectedArc3Sampler s(*arc);
        auto mid {s.sample(0.5)};
        QVERIFY(glm::all(glm::epsilonEqual(mid, glm::dvec3(9.45778790622,-6.81602687507,24.17962827310), eps)));
    }
    {
        auto arc = DirectedArc3::create3Points({10, 0, 0}, {5, 5, 0}, {0, 0, 0}, 0);
        DirectedArc3Sampler s(*arc);
        auto mid {s.sample(0.5)};
        QVERIFY(glm::all(glm::epsilonEqual(mid, glm::dvec3(5, 5, 0), eps)));
    }
}

void test_case_1::helix_sampling()
{
    const double eps {1e-14};
    {
        auto arc = DirectedArc2::create2PointsRadius({5,5}, {10,10}, 5, DirectedArc2::clw, 0);
        Helix helix {arc.value(), glm::dmat4(1.0), 0.0, 5.0, 1};
        HelixSampler s {helix};
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.2), glm::dvec3(10,10,1), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.4), glm::dvec3(15,5,2), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.6), glm::dvec3(10,0,3), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(0.8), glm::dvec3(5,5,4), eps)));
        QVERIFY(glm::all(glm::epsilonEqual(s.sample(1.0), glm::dvec3(10,10,5), eps)));
    }
}

QTEST_APPLESS_MAIN(test_case_1)

#include "tst_test_case_1.moc"
