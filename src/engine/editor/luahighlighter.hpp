#pragma once

#include <QSyntaxHighlighter>

class LuaHighlighter : public QSyntaxHighlighter
{
public:
  LuaHighlighter(QTextDocument* parent = 0);

protected:
  void highlightBlock(const QString& text) Q_DECL_OVERRIDE;

private:
  struct HighlightingRule
  {
    QRegExp pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QRegExp commentStartExpression;
  QRegExp commentEndExpression;
  QRegExp quoteStartExpression;
  QRegExp quoteEndExpression;

  QTextCharFormat keywordFormat;
  QTextCharFormat valueFormat;
  QTextCharFormat singleLineCommentFormat;
  QTextCharFormat quotationFormat;
  QTextCharFormat functionFormat;
};