#ifndef BACKPLOTWIDGET_H
#define BACKPLOTWIDGET_H

#include "motion.h"
#include "orthographicviewwidget.h"

#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>

#include <array>
#include <ostream>


class QOpenGLShaderProgram;

class BackplotWidget : public OrthographicViewWidget, protected QOpenGLFunctions
{
    //Q_OBJECT
public:
    explicit BackplotWidget(QWidget* parent = nullptr);
    ~BackplotWidget();

    void clear();

    // QOpenGLWidget interface
    void initializeGL() override;
    void paintGL() override;

    void startTrajectory(const glm::vec3& startPoint);
    void plot(const LinearMotion& motion);
    void plot(const CircularMotion& motion);
    void plot(const HelicalMotion& motion);
    void endTrajectory();

private:
    void addPoint(const glm::vec3& point, const std::array<GLubyte, 3>& color);
    void saveOffset();
    
    QOpenGLVertexArrayObject m_backgroundVao;
    QOpenGLBuffer m_backgroundBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLShaderProgram* m_backgroundShaderProgram{nullptr};

    QOpenGLVertexArrayObject m_trajectoryVao;
    QOpenGLBuffer m_trajectoryBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLShaderProgram* m_trajectoryShaderProgram{nullptr};

    struct Vertex
    {
        glm::vec3 position;
        GLubyte color[3];
        Vertex() = default;

        Vertex(const glm::vec3& vec, unsigned char r=0, unsigned char g=0, unsigned char b=0);

        friend std::ostream& operator<<(std::ostream& os, Vertex& v)
        {
            return os << '{' << "x:" << v.position[0] << " y:" << v.position[1] << " z:" << v.position[2]  << '}';
        }
    };

    std::vector<Vertex> m_vertices;
    std::vector<size_t> m_offsets;
    bool m_trajectoryChange {false};
    bool m_repaintLocked {false};

    BoundingBox m_boundingBox;
    std::array<Vertex, 24> m_boundingBoxVertices;

    const std::array<GLubyte, 3> m_colorRapid {255, 50, 50};
    const std::array<GLubyte, 3> m_colorLinear {50, 255, 50};
    const std::array<GLubyte, 3> m_colorCircular {40, 180, 255};
};


#endif // BACKPLOTWIDGET_H
