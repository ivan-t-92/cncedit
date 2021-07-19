#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "controller.h"

#include <QPlainTextEdit>

class Highlighter;
class BackplotWidget;


class CodeEditor : public QPlainTextEdit, public ControllerListener
{
    Q_OBJECT

public:
    explicit CodeEditor(BackplotWidget& backplot, QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();

    enum class ColorHintType
    {
        Unset,
        RapidMotion,
        LinearMotion,
        CircularMotion,
        NoMotion,
    };

    void setLineColorHint(size_t lineNumber, ColorHintType colorHintType);
    void clearLineColorHints();

    // ControllerListener interface
    void startPoint(const glm::dvec3& point) override;
    void blockChange(size_t blockNumber) override;
    void linearMotion(const LinearMotion& linearMotion) override;
    void circularMotion(const CircularMotion& circularMotion) override;
    void helicalMotion(const HelicalMotion& helicalMotion) override;
    void endOfProgram() override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);

private:
    void onBlockCountChange();
    void onDocumentChange();
    BackplotWidget& m_backplot;
    QWidget* lineNumberArea;
    Highlighter* highlighter;
    QFontDatabase fontDatabase;
    static constexpr int motionColorHintLineWidth {3}; // in pixels
    std::vector<ColorHintType> m_colorHints;
    Controller m_controller;
    size_t m_currentBlockNumber {};
};


class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor* editor)
        : QWidget(editor),
          m_editor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor* m_editor;
};

#endif // CODEEDITOR_H
