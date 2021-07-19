#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <glm/vec3.hpp>

#include <array>


class BoundingBox
{
public:
    typedef std::array<glm::vec3, 8> corners_t;
    enum CornerIndex
    {
        lower = 0,
        lower_upperX,
        lower_upperY,
        lower_upperZ,
        upper_lowerX,
        upper_lowerY,
        upper_lowerZ,
        upper
    };

    BoundingBox() = default;
    BoundingBox(const glm::vec3& point1, const glm::vec3& point2) noexcept;
    void include(const glm::vec3& point) noexcept;
    void reset() noexcept;

    const glm::vec3& lowerCorner() const noexcept { return m_corners[lower]; }
    const glm::vec3& upperCorner() const noexcept { return m_corners[upper]; }
    glm::vec3 centerPoint() const noexcept;
    const corners_t& corners() noexcept;

    bool isDefined() const noexcept { return m_defined; }

private:
    corners_t m_corners;
    bool m_defined = false;
    bool m_cornersNeedRecalc = true;
};
#endif // BOUNDINGBOX_H
