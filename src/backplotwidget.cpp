#include "backplotwidget.h"
#include "motion.h"
#include "geometry.h"

#include <QOpenGLShaderProgram>
#include <QWheelEvent>
#include <QFile>

#include <glm/gtc/type_ptr.hpp>

#include <cmath>

static bool addShaderFromFile(const char *filename,
                              QOpenGLShader::ShaderType type,
                              QOpenGLShaderProgram &shaderProgram)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    bool result = shaderProgram.addShaderFromSourceCode(type, f.readAll());
    f.close();
    return result;
}

struct BackgroundVertex
{
    struct Point
    {
        std::array<float, 2> p;
    };
    struct Color
    {
        std::array<GLubyte, 3> c;
    };

    Point point;
    Color color;
};

BackplotWidget::BackplotWidget(QWidget* parent)
    : OrthographicViewWidget(parent)
{
    // No OpenGL resource initialization is done here.
}

BackplotWidget::~BackplotWidget()
{
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();
    
    m_backgroundVao.destroy();
    m_backgroundBuffer.destroy();
    delete m_backgroundShaderProgram;

    m_trajectoryVao.destroy();
    m_trajectoryBuffer.destroy();
    delete m_trajectoryShaderProgram;

    doneCurrent();
}


void BackplotWidget::clear()
{
    m_vertices.clear();
    m_offsets.clear();
    m_boundingBox.reset();
}

void BackplotWidget::initializeGL()
{
    initializeOpenGLFunctions();

    //std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    //std::cout << "OpenGL GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    m_backgroundVao.create();
    if (m_backgroundVao.isCreated())
        m_backgroundVao.bind();
    
    m_backgroundBuffer.create();
    m_backgroundBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_backgroundBuffer.bind();

    BackgroundVertex::Point topLeft {-1.0f, 1.0f};
    BackgroundVertex::Point topRight {1.0f, 1.0f};
    BackgroundVertex::Point bottomLeft {-1.0f, -1.0f};
    BackgroundVertex::Point bottomRight {1.0f, -1.0f};
    BackgroundVertex::Color topColor {170, 180, 190};
    BackgroundVertex::Color bottomColor {210, 220, 230};
    BackgroundVertex backgroundBuffer[] {
        {topLeft,     topColor},
        {topRight,    topColor},
        {bottomRight, bottomColor},
        {bottomRight, bottomColor},
        {bottomLeft,  bottomColor},
        {topLeft,     topColor}
    };
    m_backgroundBuffer.allocate(backgroundBuffer, sizeof(backgroundBuffer));

    glVertexAttribPointer(0,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(BackgroundVertex),
                          (const void *) offsetof(BackgroundVertex, point));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,
                          3,
                          GL_UNSIGNED_BYTE,
                          GL_TRUE,
                          sizeof(BackgroundVertex),
                          (const void *) offsetof(BackgroundVertex, color));
    glEnableVertexAttribArray(1);

    m_backgroundShaderProgram = new QOpenGLShaderProgram();
    addShaderFromFile(":/shaders/background.vert", QOpenGLShader::Vertex, *m_backgroundShaderProgram);
    addShaderFromFile(":/shaders/background.frag", QOpenGLShader::Fragment, *m_backgroundShaderProgram);
    m_backgroundShaderProgram->link();

    m_backgroundVao.release();


    m_trajectoryVao.create();
    if (m_trajectoryVao.isCreated())
        m_trajectoryVao.bind();

    m_trajectoryBuffer.create();
    m_trajectoryBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_trajectoryBuffer.bind();

    // QOpenGLShaderProgram::setAttributeBuffer does not support normalization parameter
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (const void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    m_trajectoryShaderProgram = new QOpenGLShaderProgram();
    addShaderFromFile(":/shaders/trajectory.vert", QOpenGLShader::Vertex, *m_trajectoryShaderProgram);
    addShaderFromFile(":/shaders/trajectory.frag", QOpenGLShader::Fragment, *m_trajectoryShaderProgram);
    m_trajectoryShaderProgram->link();

    m_trajectoryVao.release();
}

void BackplotWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_repaintLocked)
        return;

    // Draw background
    glDisable(GL_DEPTH_TEST);
    m_backgroundVao.bind();
    m_backgroundShaderProgram->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_backgroundVao.release();

    // Draw trajectory and bounding box
    glEnable(GL_DEPTH_TEST);

    m_trajectoryVao.bind();
    m_trajectoryShaderProgram->bind();

    int mvpLocation = m_trajectoryShaderProgram->uniformLocation("uMVP");
    if (mvpLocation == -1)
        throw std::runtime_error("uniform uMVP not found");
    //m_trajectoryShaderProgram->setUniformValue(mvpLocation, glm::value_ptr(calcViewProjectionMatrix()));
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(calcViewProjectionMatrix()));

    if (m_trajectoryChange)
    {
        auto trajByteCount {static_cast<int>(m_vertices.size() * sizeof(Vertex))};
        auto bboxByteCount {static_cast<int>(m_boundingBoxVertices.size() * sizeof(Vertex))};

        m_trajectoryBuffer.allocate(trajByteCount + bboxByteCount);
        m_trajectoryBuffer.write(0, m_vertices.data(), trajByteCount);
        if (m_boundingBox.isDefined())
            m_trajectoryBuffer.write(trajByteCount, m_boundingBoxVertices.data(), bboxByteCount);

        m_trajectoryChange = false;
    }
    if (m_vertices.size() > 1)
    {
        glDrawArrays(GL_LINE_STRIP_ADJACENCY, 0, static_cast<GLsizei>(m_vertices.size()));
        if (m_boundingBox.isDefined())
            glDrawArrays(GL_LINES, static_cast<GLint>(m_vertices.size()),
                         static_cast<GLsizei>(m_boundingBoxVertices.size()));
    }

    m_trajectoryVao.release();
}

void BackplotWidget::addPoint(const glm::vec3& point, const std::array<GLubyte, 3>& color)
{
    m_vertices.emplace_back(point, color[0], color[1], color[2]);
    m_boundingBox.include(point);
}

void BackplotWidget::saveOffset()
{
    m_offsets.push_back(m_vertices.size());
}

void BackplotWidget::startTrajectory(const glm::vec3& startPoint)
{
    m_repaintLocked = true;
    clear();
    addPoint(startPoint, {0, 0, 0});
    // duplicate first vertex for geometry shader processing LINE_STRIP_ADJACENCY
    m_vertices.emplace_back(m_vertices.front());
}

void BackplotWidget::plot(const LinearMotion& motion)
{
    saveOffset();
    addPoint(motion.getEndPoint(), (motion.getFeed() > 0) ? m_colorLinear : m_colorRapid);
}

void BackplotWidget::plot(const CircularMotion& motion)
{
    saveOffset();
    const auto color {(motion.getFeed() > 0) ? m_colorCircular : m_colorRapid};
    DirectedArc3Sampler s {motion.getArc()};
    const int arcPoints {100}; // TODO adaptive precision
    for (int i {1}; i < arcPoints; i++)
        addPoint(s.sample((double)i / (double)(arcPoints - 1)), color);
}

void BackplotWidget::plot(const HelicalMotion& motion)
{
    saveOffset();
    const auto color {(motion.getFeed() > 0) ? m_colorCircular : m_colorRapid};
    HelixSampler s {motion.getHelix()};
    const int arcPoints = 100 * (motion.getHelix().turn + 1); // TODO adaptive precision
    for (int i {1}; i < arcPoints; i++)
        addPoint(s.sample((double)i / (double)(arcPoints - 1)), color);
}

void BackplotWidget::endTrajectory()
{
    m_trajectoryChange = true;
    m_repaintLocked = false;

    // duplicate last vertex for geometry shader processing LINE_STRIP_ADJACENCY
    m_vertices.emplace_back(m_vertices.back());

    if (m_boundingBox.isDefined())
    {
        auto it = m_boundingBoxVertices.begin();
#define ADD_POINT(ID) *it++ = {m_boundingBox.corners()[ ID ], 100, 100, 100};
        ADD_POINT(BoundingBox::lower)
        ADD_POINT(BoundingBox::lower_upperX)
        ADD_POINT(BoundingBox::lower)
        ADD_POINT(BoundingBox::lower_upperY)
        ADD_POINT(BoundingBox::lower)
        ADD_POINT(BoundingBox::lower_upperZ)
        ADD_POINT(BoundingBox::lower_upperX)
        ADD_POINT(BoundingBox::upper_lowerZ)
        ADD_POINT(BoundingBox::lower_upperX)
        ADD_POINT(BoundingBox::upper_lowerY)
        ADD_POINT(BoundingBox::lower_upperY)
        ADD_POINT(BoundingBox::upper_lowerX)
        ADD_POINT(BoundingBox::lower_upperY)
        ADD_POINT(BoundingBox::upper_lowerZ)
        ADD_POINT(BoundingBox::upper)
        ADD_POINT(BoundingBox::upper_lowerX)
        ADD_POINT(BoundingBox::upper)
        ADD_POINT(BoundingBox::upper_lowerY)
        ADD_POINT(BoundingBox::upper)
        ADD_POINT(BoundingBox::upper_lowerZ)
        ADD_POINT(BoundingBox::lower_upperZ)
        ADD_POINT(BoundingBox::upper_lowerX)
        ADD_POINT(BoundingBox::lower_upperZ)
        ADD_POINT(BoundingBox::upper_lowerY)
#undef ADD_POINT
        setSceneBoundingBox(m_boundingBox);
        setPivotPoint(m_boundingBox.centerPoint());
    }

    update();
}

BackplotWidget::Vertex::Vertex(const glm::vec3& vec, unsigned char r, unsigned char g, unsigned char b)
    : position{vec.x, vec.y, vec.z},
      color{r, g, b}
{}
