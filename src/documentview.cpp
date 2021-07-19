#include "documentview.h"
#include "codeeditor.h"
#include "backplotwidget.h"

#include <QTextBlock>


DocumentView::DocumentView(const QString& text, QWidget* parent)
    : QSplitter(parent),
      m_backplotWidget(new BackplotWidget),
      m_codeEditor(new CodeEditor(*m_backplotWidget))
{
    addWidget(m_codeEditor);
    addWidget(m_backplotWidget);

    document()->setPlainText(text);
    document()->setModified(false);
}

QTextDocument* DocumentView::document() const
{
    return m_codeEditor->document();
}



