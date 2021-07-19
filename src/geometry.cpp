#include "geometry.h"

#include <cmath>

static glm::dvec2 intersect(const glm::dvec2& p1, const glm::dvec2& p2, const glm::dvec2& p3, const glm::dvec2& p4)
{
    auto d {(p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x)};
    return {((p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x)) / d,
            ((p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x)) / d};
}

static glm::dvec2 normal(const glm::dvec2& p1, const glm::dvec2& p2, bool right)
{
    auto n {glm::normalize(p2 - p1)};
    if (right)
        return {n.y, -n.x};
    else
        return {-n.y, n.x};
}

static glm::dvec2 midpoint(const glm::dvec2& p1, const glm::dvec2& p2)
{
    return (p1 + p2) * 0.5;
}

std::optional<DirectedArc2> DirectedArc2::create2PointsCenter(const glm::dvec2& center,
                                                              const glm::dvec2& point1,
                                                              const glm::dvec2& point2,
                                                              DirectedArc2::ArcDirection dir,
                                                              double tolerance)
{
    if (glm::all(glm::epsilonEqual(point1, point2, 1e-12)))
        return DirectedArc2{center, point2, point2, dir};

    auto dist1 {glm::distance(center, point1)};
    auto dist2 {glm::distance(center, point2)};

    if (std::abs(dist1 - dist2) > tolerance)
        return std::nullopt;

    auto radius {(glm::length(dist1) + glm::length(dist2)) * 0.5};

    constexpr auto eps {1e-14};
    auto arc1 {create2PointsRadius(point1, point2, radius, dir, eps)};
    auto arc2 {create2PointsRadius(point1, point2, -radius, dir, eps)};
    if (arc1.has_value() && arc2.has_value())
    {
        auto dr1 {glm::distance(center, arc1->center)};
        auto dr2 {glm::distance(center, arc2->center)};
        return dr1 < dr2 ? arc1 : arc2;
    }

    return std::nullopt;
}

std::optional<DirectedArc2> DirectedArc2::create2PointsRadius(const glm::dvec2& point1,
                                                              const glm::dvec2& point2,
                                                              double radius,
                                                              DirectedArc2::ArcDirection dir,
                                                              double tolerance)
{
    if (radius == 0.0)
        return std::nullopt;

    auto pmid {midpoint(point1, point2)};
    auto dist {glm::distance(point1, point2)};
    glm::dvec2 center;

    if (auto dia {2 * std::abs(radius)};
        dia < dist)
    {
        if (dist - dia > tolerance)
            return std::nullopt;

        center = pmid;
    }
    else
    {
        auto diffNormalized {(point2 - point1) / dist};
        glm::dvec2 normal;
        if ((radius > 0) ^ (dir == clw))
            normal = {-diffNormalized.y, diffNormalized.x}; // rotate 90 degrees cclw
        else
            normal = {diffNormalized.y, -diffNormalized.x}; // rotate 90 degrees clw
        center = pmid + normal * std::sqrt(radius * radius - dist * dist * 0.25);
    }

    return DirectedArc2 {center, point1, point2, dir};
}

std::optional<DirectedArc2> DirectedArc2::create3Points(const glm::dvec2& point1,
                                                        const glm::dvec2& point2,
                                                        const glm::dvec2& point3,
                                                        double tolerance)
{
    auto n1 {normal(point1, point2, false)};
    auto n2 {normal(point2, point3, false)};
    auto m1 {midpoint(point1, point2)};
    auto m2 {midpoint(point2, point3)};

    auto center {intersect(m1, m1 + n1, m2, m2 + n2)};

    auto angle {glm::orientedAngle(glm::normalize(point3 - point1),
                                   glm::normalize(point2 - point1))};
    ArcDirection dir {angle < 0 ? cclw : clw};

    return DirectedArc2 {center, point1, point3, dir};
}

std::optional<DirectedArc3> DirectedArc3::create3Points(const glm::dvec3& point1,
                                                        const glm::dvec3& point2,
                                                        const glm::dvec3& point3,
                                                        double tolerance)
{
    auto z {glm::triangleNormal(point1, point2, point3)};
    auto x {glm::normalize(point3 - point1)};
    auto y {glm::cross(z, x)};

    glm::dmat3 rot(x, y, z);
    auto transl {glm::translate(glm::dmat4(1.0), point1)};
    auto transf {transl * glm::dmat4(rot)};
    auto invTransf {glm::inverse(transf)};
    glm::dvec2 point1_2d {invTransf * glm::dvec4(point1, 1.0)};
    glm::dvec2 point2_2d {invTransf * glm::dvec4(point2, 1.0)};
    glm::dvec2 point3_2d {invTransf * glm::dvec4(point3, 1.0)};

    auto arc2 {DirectedArc2::create3Points(point1_2d, point2_2d, point3_2d, tolerance)};
    if (!arc2.has_value())
        return std::nullopt;

    return DirectedArc3 {arc2.value(), transf, 0};
}

DirectedArc2Sampler::DirectedArc2Sampler(const DirectedArc2& arc2)
    : m_arc2(arc2),
      m_centerToPoint1(arc2.point1 - arc2.center),
      m_angle(angle(m_centerToPoint1, arc2.point2 - arc2.center, arc2.dir))
{}

glm::dvec2 DirectedArc2Sampler::sample(const double param) const
{
    //assert(param >= 0.0 && param <= 1.0);
    return m_arc2.center + glm::rotate(m_centerToPoint1, m_angle * param);
}

double DirectedArc2Sampler::angle(const glm::dvec2& v1, const glm::dvec2& v2, const DirectedArc2::ArcDirection dir)
{
    auto a {glm::orientedAngle(glm::normalize(v1), glm::normalize(v2))};
    if (std::abs(a) <= eps)
    {
        switch (dir)
        {
        case DirectedArc2::cclw:
            a = 2 * glm::pi<double>();
            break;
        case DirectedArc2::clw:
            a = -2 * glm::pi<double>();
            break;
        }
    }
    else
    {
        switch (dir)
        {
        case DirectedArc2::cclw:
            if (a < 0)
                a += 2 * glm::pi<double>();
            break;
        case DirectedArc2::clw:
            if (a > 0)
                a -= 2 * glm::pi<double>();
            break;
        }
    }
    return a;
}

DirectedArc3Sampler::DirectedArc3Sampler(const DirectedArc3& arc3)
    : m_arc3(arc3),
      m_arc2sampler(arc3.arc2)
{}

glm::dvec3 DirectedArc3Sampler::sample(const double param) const
{
    auto v {m_arc2sampler.sample(param)};
    return m_arc3.transform * glm::dvec4(v, m_arc3.z, 1.0);
}

HelixSampler::HelixSampler(const Helix& helix)
    : m_helix(helix),
      m_arc2sampler(helix.arc2)
{}

glm::dvec3 HelixSampler::sample(const double param) const
{
    auto z {m_helix.zStart + (m_helix.zEnd - m_helix.zStart) * param};
    double sampleParam;
    if (m_helix.turn > 0)
    {
        auto turnAngle {std::copysign(m_helix.turn * 2 * glm::pi<double>(), m_arc2sampler.m_angle)};
        auto totalAngle {turnAngle + m_arc2sampler.m_angle};
        auto paramLastArcStart {1.0 - m_arc2sampler.m_angle / totalAngle};
        if (param > paramLastArcStart)
            sampleParam = (param - paramLastArcStart) / (1.0 - paramLastArcStart);
        else
            sampleParam = param / paramLastArcStart * (2 * glm::pi<double>() / std::abs(m_arc2sampler.m_angle)) * m_helix.turn;
    }
    else
        sampleParam = param;

    return m_helix.transform * glm::dvec4(m_arc2sampler.sample(sampleParam), z, 1.0);
}
