#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/normal.hpp>
#include <glm/gtc/constants.hpp>

#include <optional>


struct DirectedArc2
{
    enum ArcDirection
    {
        clw,
        cclw
    };

    DirectedArc2() = delete;

    const glm::dvec2 center;
    const glm::dvec2 point1;
    const glm::dvec2 point2;
    const ArcDirection dir;

    static std::optional<DirectedArc2> create2PointsCenter(const glm::dvec2& center,
                                                           const glm::dvec2& point1,
                                                           const glm::dvec2& point2,
                                                           ArcDirection dir,
                                                           double tolerance);

    static std::optional<DirectedArc2> create2PointsRadius(const glm::dvec2& point1,
                                                           const glm::dvec2& point2,
                                                           double radius,
                                                           ArcDirection dir,
                                                           double tolerance);

    static std::optional<DirectedArc2> create3Points(const glm::dvec2& point1,
                                                     const glm::dvec2& point2,
                                                     const glm::dvec2& point3,
                                                     double tolerance);
};

struct DirectedArc3
{
    const DirectedArc2 arc2;
    const glm::dmat4 transform;
    const double z;

    static std::optional<DirectedArc3> create3Points(const glm::dvec3& point1,
                                                     const glm::dvec3& point2,
                                                     const glm::dvec3& point3,
                                                     double tolerance);
};

struct Helix
{
    const DirectedArc2 arc2;
    const glm::dmat4 transform;
    const double zStart;
    const double zEnd;
    const unsigned turn;
};

class DirectedArc2Sampler
{
    const DirectedArc2& m_arc2;
    const glm::dvec2 m_centerToPoint1;
    const double m_angle;
    friend class HelixSampler;
public:
    explicit DirectedArc2Sampler(const DirectedArc2& arc2);

    glm::dvec2 sample(const double param) const;

private:
    constexpr static double eps {1e-10};

    static double angle(const glm::dvec2& v1, const glm::dvec2& v2, const DirectedArc2::ArcDirection dir);
};

class DirectedArc3Sampler
{
    const DirectedArc3& m_arc3;
    const DirectedArc2Sampler m_arc2sampler;

public:
    explicit DirectedArc3Sampler(const DirectedArc3& arc3);

    glm::dvec3 sample(const double param) const;
};

class HelixSampler
{
    const Helix& m_helix;
    const DirectedArc2Sampler m_arc2sampler;

public:
    explicit HelixSampler(const Helix& helix);

    glm::dvec3 sample(const double param) const;
};

#endif // GEOMETRY_H
