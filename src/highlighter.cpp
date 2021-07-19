#include "highlighter.h"
#include "parser.h"

#include <QRegularExpression>


Highlighter::Highlighter(QTextDocument* doc)
    : QSyntaxHighlighter(doc)
{
    highlightingRules.reserve(10);

    HighlightingRule rule;
    QTextCharFormat format;

    format.setForeground(QColor(0xCD0000));
    rule.pattern = QRegularExpression(R"((?:\b|\d)([gG]0+)(?=\D|$))");
    rule.nth = 1;
    rule.format = format;
    highlightingRules.push_back(rule);

    format.setForeground(QColor(0x14AA28));
    rule.pattern = QRegularExpression(R"((?:\b|\d)([gG]0*1)(?=\D|$))");
    rule.nth = 2;
    rule.format = format;
    highlightingRules.push_back(rule);

    format.setForeground(QColor(0x00C8F0));
    rule.pattern = QRegularExpression(R"((?:\b|\d)([gG]0*[23])(?=\D|$))");
    rule.nth = 2;
    rule.format = format;
    highlightingRules.push_back(rule);

    format.setForeground(QColor(0x004BAF));
    rule.pattern = QRegularExpression(R"((?:\b|\d)([dD]\d{1,3})(?=\D|$))");
    rule.nth = 2;
    rule.format = format;
    highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"((?:\b|\d)([dD][lL](=(QU\(_+\)|[0-6])|\[[_+]\]=([-+]?\d+[.]?\d*(?:EX[-+]?\d+)?|QU\(_+\))))(?=\D|$))");
    highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"((?:\b|\d)([sS](\d{1,5}|(\d*|\[[A-Za-z_]\w{0,30}\])=(\d+|[A-Za-z_]\w{0,30}(\[_+\])?|QU\(_+\))))(?=\D|$))");
    highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"((?:\b|\d)([fF][zZ]?(=?(\d+(\.\d+)?)|=QU\(_+\)))(?=\D|$))");
    highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"((?:\b|\d)([mM](\d{1,10}|(\d*|\[[A-Za-z_]\w{0,30}\])=(\d+|QU\(_+\))))(?=\D|$))");
    highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"((?:\b|\d)([hH](\d{1,10}|(\d*|\[[A-Za-z_]\w{0,30}\])=([-+]?\d+[.]?\d*(?:EX[-+]?\d+)?|QU\(_+\))))(?=\D|$))");
    highlightingRules.push_back(rule);

    rule.pattern = QRegularExpression(R"((?:\b|\d)([tT](\d+(=\d+)?|(\[[A-Za-z_]\w{0,30}\])?=("[^"]*"|\d+|QU\(_+\))))(?=\D|$))");
    highlightingRules.push_back(rule);

    m_commentFormat.setForeground(QColor(0x828C96));
}

void Highlighter::highlightBlock(const QString& text)
{
    constexpr auto captureGroup = 1;
    for (auto& rule : highlightingRules)
    {
        QRegularExpressionMatchIterator matchIterator {rule.pattern.globalMatch(text)};
        while (matchIterator.hasNext())
        {
            const QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(captureGroup), match.capturedLength(captureGroup), rule.format);
        }
    }

    int commentPos {static_cast<int>(Parser::findCommentStartPos<QString>(text.begin(), text.end()))};
    if (commentPos < text.length())
    {
        setFormat(commentPos, text.length() - commentPos, m_commentFormat);
    }
}
