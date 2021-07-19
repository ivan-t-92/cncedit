#include "orthographicviewwidget.h"

#include <QMouseEvent>
#include <glm/gtc/matrix_transform.hpp>

OrthographicViewWidget::OrthographicViewWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    updateScreenMatrix();
    updateScaleMatrix();
}


void OrthographicViewWidget::wheelEvent(QWheelEvent* event)
{
    const QPoint pixelDelta = event->pixelDelta();
    const QPoint angleDelta = event->angleDelta();

    if (!pixelDelta.isNull())
    {
        // TODO
    }
//    else
    if (angleDelta.y() != 0)
    {
        const int delta {angleDelta.y()};
        const float steps {abs((float)delta / 120.0f)};
        const float scale_change_per_step {0.25f};
        float scale_change {1 + scale_change_per_step * steps};
        if (delta < 0)
            scale_change = 1.0f / scale_change;

        const glm::vec3 zero = glm::inverse(m_view) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        const glm::vec2 deltaNDC {screenToNDC(event->posF()) - worldToNDC(zero)};

        m_scaleFactor *= scale_change;
        updateScaleMatrix();

        m_trans -= (deltaNDC * (scale_change - 1.0f));
        updateScreenMatrix();

        updateNearFar();

        event->accept();
        update();
    }
}

void OrthographicViewWidget::mousePressEvent(QMouseEvent* event)
{
    m_mousePressPos = event->pos();
}

void OrthographicViewWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_mousePressPos;

    if (event->buttons() & Qt::LeftButton)
    {
        const float sensFactor {0.01f};
        const float yRot {(float)delta.x() * sensFactor};
        const float xRot {(float)delta.y() * sensFactor};


        const glm::dmat4 rotInv = glm::inverse(m_rot);
        const glm::vec3 xScreen {rotInv * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}};
        const glm::vec3 yScreen {rotInv * glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}};

        m_rot = glm::rotate(m_rot, xRot, xScreen);
        m_rot = glm::rotate(m_rot, yRot, yScreen);

        updateViewMatrix();
        updateNearFar();
    }
    else if (event->buttons() & Qt::RightButton)
    {
        glm::vec2 deltaNDC {screenToNDC(event->pos()) - screenToNDC(m_mousePressPos)};

        m_trans += deltaNDC;
        updateScreenMatrix();
    }
    m_mousePressPos = event->pos();
    update();
}

glm::vec2 OrthographicViewWidget::worldToPixel(const glm::vec2& world) const
{
    return {world.x * (float)size().width() / 2.0f * m_scaleFactor,
            world.y / (float)size().height() / 2.0f * m_scaleFactor};
}

void OrthographicViewWidget::setPivotPoint(const glm::vec3& pivotPoint)
{
    auto zero = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
    const glm::vec2 zeroProjBefore {calcViewProjectionMatrix() * zero};
    m_pivotPoint = pivotPoint;
    updateViewMatrix();
    const glm::vec2 zeroProjAfter {calcViewProjectionMatrix() * zero};
    m_trans += zeroProjBefore - zeroProjAfter;
    updateScreenMatrix();
    updateNearFar();
}

void OrthographicViewWidget::setSceneBoundingBox(const BoundingBox& b)
{
    m_boundingBox = b;
    updateNearFar();
}

void OrthographicViewWidget::resizeGL(int, int)
{
    updateProjMatrix(m_near, m_far);
}

float OrthographicViewWidget::calcAspectRatio() const
{
    return size().height() == 0 ? 1 : (float)size().width() / (float)size().height();
}

void OrthographicViewWidget::updateScreenMatrix()
{
    m_screen = glm::translate(glm::mat4(1.0f), glm::vec3(m_trans, 0.0f));
    invalidateVPMatrix();
}

void OrthographicViewWidget::updateProjMatrix(float _near, float _far)
{
    m_near = _near;
    m_far = _far;
    const float aspectRatio = calcAspectRatio();
    m_proj = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, _near, _far);
    invalidateVPMatrix();
}

void OrthographicViewWidget::updateViewMatrix()
{
    m_view = glm::translate(glm::mat4(1.0f), m_pivotPoint);
    m_view = m_view * m_rot;
    m_view = glm::translate(m_view, -m_pivotPoint);
    invalidateVPMatrix();
}

void OrthographicViewWidget::updateScaleMatrix()
{
    m_scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_scaleFactor));
    invalidateVPMatrix();
}

const glm::mat4& OrthographicViewWidget::calcViewProjectionMatrix()
{
    if (m_recalcVPMatrix)
    {
        m_viewProjectionMatrixCache = m_screen * m_proj * m_scale * m_view;
        m_recalcVPMatrix = false;
    }
    return m_viewProjectionMatrixCache;
}

void OrthographicViewWidget::updateNearFar()
{
    if (!m_boundingBox.isDefined())
        return;

    const glm::mat4 view = m_scale * m_view;

    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    auto& corners = m_boundingBox.corners();
    for (auto& corner : corners)
    {
        float z = (view * glm::vec4(corner, 1.0f)).z;
        zMin = std::min(zMin, z);
        zMax = std::max(zMax, z);
    }

    zMin -= 0.01f;
    zMax += 0.01f;

    updateProjMatrix(-zMax, -zMin);
}

void OrthographicViewWidget::invalidateVPMatrix()
{
    m_recalcVPMatrix = true;
}

glm::vec2 OrthographicViewWidget::worldToNDC(const glm::vec3& worldPoint)
{
    return glm::vec2(calcViewProjectionMatrix() * glm::vec4(worldPoint, 1.0));
}

glm::vec2 OrthographicViewWidget::screenToNDC(const QPointF& screenPoint) const
{
    return glm::vec2(screenPoint.x() / (float)size().width() * 2.0f - 1.0f, 1.0f - screenPoint.y() / (float)size().height() * 2.0f);
}
