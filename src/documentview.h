#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include "controller.h"

#include <QSplitter>
#include <QString>

#include <memory>

class CodeEditor;
class BackplotWidget;
class QTextDocument;

class DocumentView : public QSplitter
{
    Q_OBJECT
public:
    explicit DocumentView(const QString& text, QWidget* parent = nullptr);

    CodeEditor* editor() const { return m_codeEditor; }
    BackplotWidget* backplot() const { return m_backplotWidget; }

    QString path() const { return m_path; }
    void setPath(const QString& path) { m_path = path; }

    QTextDocument* document() const;

private:
    BackplotWidget* m_backplotWidget;
    CodeEditor* m_codeEditor;
    QString m_path;
};

#endif // DOCUMENTVIEW_H
