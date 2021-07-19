#ifndef MOTION_H
#define MOTION_H

#include "geometry.h"

#include <glm/vec3.hpp>

class Motion
{
    const double m_feed;

public:
    explicit Motion(double feed)
        : m_feed(feed)
    {}

    inline double getFeed() const { return m_feed; }
};

class LinearMotion : public Motion
{
    const glm::dvec3 m_endPoint;

public:
    explicit LinearMotion(const glm::dvec3& endPoint, double feed)
        : Motion(feed),
          m_endPoint(endPoint)
    {}

    const glm::dvec3& getEndPoint() const { return m_endPoint; }
};

class CircularMotion : public Motion
{
    const DirectedArc3 m_arc3;

public:
    explicit CircularMotion(const DirectedArc3& arc3, double feed)
        : Motion(feed),
          m_arc3(arc3)
    {}

    const DirectedArc3& getArc() const { return m_arc3; }
};

class HelicalMotion : public Motion
{
    const Helix m_helix;

public:
    explicit HelicalMotion(const Helix& helix, double feed)
        : Motion(feed),
          m_helix(helix)
    {}

    const Helix& getHelix() const { return m_helix; }
};

#endif // MOTION_H
