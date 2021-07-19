#ifndef ORTHOGRAPHICRENDERER_H
#define ORTHOGRAPHICRENDERER_H

#include <QOpenGLWidget>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "boundingbox.h"

/**
 * Base class for displaying an OpenGL-rendered scene with an orthographic projection.
 * Supports scene zoom with mouse wheel, panning and rotating around a given point in
 * model space.
 */
class OrthographicViewWidget : public QOpenGLWidget
{
public:
    explicit OrthographicViewWidget(QWidget* parent = nullptr);

    const glm::mat4& calcViewProjectionMatrix();
    float nearPlane() const { return m_near; }
    float farPlane() const { return m_far; }
    void setPivotPoint(const glm::vec3& pivotPoint);

    // set bounding box for automatic calculation of near and far planes
    // depending on camera position and rotation
    void setSceneBoundingBox(const BoundingBox& b);

    void resizeGL(int, int) override;

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // camera position and rotation matrix.
    // Note: position of the camera is always determined by
    // rotating the origin (0,0,0) around the pivot point
    glm::mat4 m_view {1.0f};

    // orthographic scale matrix, separate from the view matrix
    glm::mat4 m_scale {1.0f};

    // orthographic projection matrix
    glm::mat4 m_proj {1.0f};

    // projected view offset
    glm::mat4 m_screen {1.0f};

    float m_scaleFactor {0.01f};
    float m_near {-1.0f};
    float m_far {1.0f};

    QPoint m_mousePressPos;
    glm::vec2 m_trans {0.0f};
    glm::mat4 m_rot {1.0f};

    BoundingBox m_boundingBox;
    glm::vec3 m_pivotPoint {0.0f};

    glm::mat4 m_viewProjectionMatrixCache {1.0f};
    bool m_recalcVPMatrix {true};

    glm::vec2 worldToNDC(const glm::vec3& worldPoint);
    glm::vec2 screenToNDC(const QPointF& screenPoint) const;
    glm::vec2 worldToPixel(const glm::vec2& world) const;

    float calcAspectRatio() const;
    void updateScreenMatrix();
    void updateProjMatrix(float _near, float _far);
    void updateViewMatrix();
    void updateScaleMatrix();
    void updateNearFar();
    void invalidateVPMatrix();
};

#endif // ORTHOGRAPHICRENDERER_H
