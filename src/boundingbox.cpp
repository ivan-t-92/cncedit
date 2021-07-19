#include "boundingbox.h"

#include <iostream>

constexpr static glm::vec3 min(const glm::vec3& point1, const glm::vec3& point2) noexcept
{
    return { std::min(point1.x, point2.x),
             std::min(point1.y, point2.y),
             std::min(point1.z, point2.z) };
}

constexpr static glm::vec3 max(const glm::vec3& point1, const glm::vec3& point2) noexcept
{
    return { std::max(point1.x, point2.x),
             std::max(point1.y, point2.y),
             std::max(point1.z, point2.z) };
}

BoundingBox::BoundingBox(const glm::vec3& point1, const glm::vec3& point2) noexcept
    : m_defined(true)
{
    m_corners[lower] = min(point1, point2);
    m_corners[upper] = max(point1, point2);
}

void BoundingBox::include(const glm::vec3& point) noexcept
{
    if (m_defined)
    {
        m_corners[lower] = min(m_corners[lower], point);
        m_corners[upper] = max(m_corners[upper], point);
    }
    else
    {
        m_corners[lower] = point;
        m_corners[upper] = point;
        m_defined = true;
    }
    m_cornersNeedRecalc = true;
}

void BoundingBox::reset() noexcept
{
    m_defined = false;
    m_corners[lower] = {};
    m_corners[upper] = {};
}

glm::vec3 BoundingBox::centerPoint() const noexcept
{
    if (!m_defined)
        std::cerr << "BoundingBox is undefined" << std::endl;

    return (m_corners[lower] + m_corners[upper]) * 0.5f;
}

const BoundingBox::corners_t& BoundingBox::corners() noexcept
{
    if (!m_defined)
        std::cerr << "BoundingBox is undefined" << std::endl;

    if (m_cornersNeedRecalc)
    {
        m_corners[lower_upperX] = m_corners[lower]; m_corners[lower_upperX].x = m_corners[upper].x;
        m_corners[lower_upperY] = m_corners[lower]; m_corners[lower_upperY].y = m_corners[upper].y;
        m_corners[lower_upperZ] = m_corners[lower]; m_corners[lower_upperZ].z = m_corners[upper].z;
        m_corners[upper_lowerX] = m_corners[upper]; m_corners[upper_lowerX].x = m_corners[lower].x;
        m_corners[upper_lowerY] = m_corners[upper]; m_corners[upper_lowerY].y = m_corners[lower].y;
        m_corners[upper_lowerZ] = m_corners[upper]; m_corners[upper_lowerZ].z = m_corners[lower].z;

        m_cornersNeedRecalc = false;
    }
    return m_corners;
}

