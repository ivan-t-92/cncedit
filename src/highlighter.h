#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

#include <vector>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument* doc = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        int nth;
        QTextCharFormat format;
    };
    std::vector<HighlightingRule> highlightingRules;
    QTextCharFormat m_commentFormat;
};

#endif // HIGHLIGHTER_H
