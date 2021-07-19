#include "codeeditor.h"
#include "highlighter.h"
#include "controller.h"
#include "backplotwidget.h"

#include <QPainter>
#include <QTextBlock>


CodeEditor::CodeEditor(BackplotWidget& backplot, QWidget* parent)
    : QPlainTextEdit(parent),
      m_backplot(backplot)
{
    m_controller.setListener(this);

    const QFont font("Source Code Pro", 12);
    setFont(font);

    setCursorWidth(2);
    setLineWrapMode(LineWrapMode::NoWrap);

    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::textChanged, this, &CodeEditor::onDocumentChange);
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::onBlockCountChange);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();

    highlighter = new Highlighter(document());

    if (!document()->isEmpty())
        onDocumentChange();
}

void CodeEditor::onDocumentChange()
{
    m_colorHints.resize(blockCount(), ColorHintType::Unset);

    clearLineColorHints();

    m_controller.reset();
    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next())
        m_controller.addLine(block.text());

    m_controller.run();
}

void CodeEditor::onBlockCountChange()
{
    updateLineNumberAreaWidth();
}

void CodeEditor::startPoint(const glm::dvec3& point)
{
    m_backplot.startTrajectory(glm::vec3(point));
}

void CodeEditor::blockChange(size_t blockNumber)
{
    m_currentBlockNumber = blockNumber;
    setLineColorHint(blockNumber, ColorHintType::NoMotion);
}

void CodeEditor::linearMotion(const LinearMotion& linearMotion)
{
    m_backplot.plot(linearMotion);
    setLineColorHint(m_currentBlockNumber, linearMotion.getFeed() == 0 ? ColorHintType::RapidMotion : ColorHintType::LinearMotion);
}

void CodeEditor::circularMotion(const CircularMotion& circularMotion)
{
    m_backplot.plot(circularMotion);
    setLineColorHint(m_currentBlockNumber, ColorHintType::CircularMotion);
}

void CodeEditor::helicalMotion(const HelicalMotion& helicalMotion)
{
    m_backplot.plot(helicalMotion);
    setLineColorHint(m_currentBlockNumber, ColorHintType::CircularMotion);
}

void CodeEditor::endOfProgram()
{
    m_backplot.endTrajectory();
}


int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = std::max(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    // at least space for 3 digits
    //digits = qMax(digits, 3);
    constexpr int padding = 4;
    return padding + motionColorHintLineWidth +
            fontMetrics().horizontalAdvance('9') * digits;
}

void CodeEditor::setLineColorHint(size_t lineNumber, ColorHintType colorHintType)
{
    if (lineNumber < m_colorHints.size())
        m_colorHints[lineNumber] = colorHintType;
}

void CodeEditor::clearLineColorHints()
{
    for (auto& colorHint : m_colorHints)
        colorHint = ColorHintType::Unset;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        //lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
        lineNumberArea->update();

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent* e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier)
    {
        const QPoint pixelDelta = event->pixelDelta();
        const QPoint angleDelta = event->angleDelta();

        if (!pixelDelta.isNull())
        {
            // TODO
        }
    //    else
        int delta = angleDelta.y();
        if (delta != 0)
        {
            const QFont& currentFont {font()};

            auto sizes {fontDatabase.pointSizes(currentFont.family())};
            const int currentSize {currentFont.pointSize()};
            int newSize = 0;

            if (delta > 0)
            {
                // find next bigger size
                auto it = std::find_if(sizes.begin(), sizes.end(),
                                       [currentSize](int i){ return i > currentSize; });
                if (it != sizes.end())
                    newSize = *it;
            }
            else
            {
                // find previous lesser size
                auto it = std::find_if(sizes.rbegin(), sizes.rend(),
                                       [currentSize](int i){ return i < currentSize; });
                if (it != sizes.rend())
                    newSize = *it;
            }
            if (newSize > 0)
            {
                QFont newFont {currentFont};
                newFont.setPointSize(newSize);
                setFont(newFont);
                updateLineNumberAreaWidth();
            }
        }
        event->accept();
    }
    else
    {
        QPlainTextEdit::wheelEvent(event);
    }
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        const QColor lineColor {QColor(Qt::lightGray).lighter(120)};

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    const QColor textColor {100, 100, 100};
    const QColor backgroundColor {220, 220, 220};

    const QColor rapidMotionColor {200, 0, 0};
    const QColor linearMotionColor {20, 170, 40};
    const QColor circularMotionColor {40, 180, 255};
    const QColor noMotionColor {0, 75, 175};

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), backgroundColor);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    int textWidth = lineNumberArea->width() - motionColorHintLineWidth - 2;

    painter.setFont(font());
    painter.setPen(textColor);
    const int fontHeight {fontMetrics().height()};
    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);

            painter.drawText(0, top, textWidth, fontHeight,
                             Qt::AlignRight | Qt::AlignVCenter, number);

            const ColorHintType motionType = m_colorHints[blockNumber];
            const QColor* color {
                motionType == ColorHintType::RapidMotion ? &rapidMotionColor :
                motionType == ColorHintType::LinearMotion ? &linearMotionColor :
                motionType == ColorHintType::CircularMotion ? &circularMotionColor :
                motionType == ColorHintType::NoMotion ? &noMotionColor :
                nullptr};
            if (color)
                painter.fillRect(textWidth + 1, top, motionColorHintLineWidth, fontHeight, *color);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
