#include "luahighlighter.hpp"

#include <QTextCharFormat>

class QTextDocument;

namespace
{
  enum
  {
    bs_none = -1,
    bs_quote = 1,
    bs_comment = 2
  };
}

LuaHighlighter::LuaHighlighter(QTextDocument *parent)
  : QSyntaxHighlighter(parent)
{
  HighlightingRule rule;

  // function calls
  functionFormat.setForeground(Qt::blue);
  rule.pattern = QRegExp(QLatin1String("\\b[A-Za-z0-9_]+(?=\\()"));
  rule.format = functionFormat;
  highlightingRules.append(rule);

  // keywords (from grammar)
  QStringList keywordPatterns;
  keywordPatterns << QLatin1String("\\bfunction\\b")
    << QLatin1String("\\bbreak\\b")
    << QLatin1String("\\bgoto\\b")
    << QLatin1String("\\bdo\\b")
    << QLatin1String("\\bend\\b")
    << QLatin1String("\\bwhile\\b")
    << QLatin1String("\\brepeat\\b")
    << QLatin1String("\\buntil\\b")
    << QLatin1String("\\bif\\b")
    << QLatin1String("\\bthen\\b")
    << QLatin1String("\\belseif\\b")
    << QLatin1String("\\belse\\b")
    << QLatin1String("\\bfor\\b")
    << QLatin1String("\\bin\\b")
    << QLatin1String("\\blocal\\b")
    << QLatin1String("\\bor\\b")
    << QLatin1String("\\band\\b")
    << QLatin1String("\\bnot\\b")
    << QLatin1String("\\breturn\\b");

  keywordFormat.setForeground(Qt::darkBlue);
  keywordFormat.setFontWeight(QFont::Bold);

  for (auto const& pattern : keywordPatterns)
  {
    rule.pattern = QRegExp(pattern);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }

  // numbers, boolean, nil
  QStringList valuePatterns;
  valuePatterns << QLatin1String("\\bnil\\b")
    << QLatin1String("\\btrue\\b")
    << QLatin1String("\\bfalse\\b")
    << QLatin1String("\\b\\d+\\b")
    << QLatin1String("\\b\\d+.\\b")
    << QLatin1String("\\b\\d+e\\b")
    << QLatin1String("\\b\\[\\dA-Fa-F]+\\b");

  valueFormat.setForeground(Qt::red);
  valueFormat.setFontWeight(QFont::Normal);

  for (auto const& pattern : valuePatterns)
  {
    rule.pattern = QRegExp(pattern);
    rule.format = valueFormat;
    highlightingRules.append(rule);
  }

  // double quote "
  quotationFormat.setForeground(Qt::darkGreen);
  rule.pattern = QRegExp(QLatin1String("\"[^\"]*\""));
  rule.format = quotationFormat;
  highlightingRules.append(rule);

  // single quote '
  rule.pattern = QRegExp(QLatin1String("\'[^\']*\'"));
  rule.format = quotationFormat;
  highlightingRules.append(rule);

  // multi line string [[ ]]
  quoteStartExpression = QRegExp(QLatin1String("\\[\\[")); // --[[
  quoteEndExpression = QRegExp(QLatin1String("\\]\\]")); // ]]

  // single line comments
  singleLineCommentFormat.setForeground(QColor(Qt::darkGray).darker(120));
  rule.pattern = QRegExp(QLatin1String("--[^\n]*"));
  rule.format = singleLineCommentFormat;
  highlightingRules.append(rule);

  //Multi Line Comment --[[ ]]
  commentStartExpression = QRegExp(QLatin1String("--\\[\\[")); // --[[
  commentEndExpression = QRegExp(QLatin1String("\\]\\]")); // ]]

  rule.pattern.setMinimal(false);
}

void LuaHighlighter::highlightBlock(const QString &text)
{
  for (auto const& rule : highlightingRules)
  {
    QRegExp expression(rule.pattern);
    int index = expression.indexIn(text);
    while (index >= 0)
    {
      int length = expression.matchedLength();
      setFormat(index, length, rule.format);
      index = expression.indexIn(text, index + length);
    }
  }

  setCurrentBlockState(bs_none);

  //
  // multi-line strings
  //

  int start = -1;
  int prev = previousBlockState();

  if (prev == bs_quote)
  {
    start = 0;
  }
  if (prev == bs_none)
  {
    start = quoteStartExpression.indexIn(text);
  }

  while (start >= 0)
  {
    int end = quoteEndExpression.indexIn(text, start);
    int length;

    if (end == -1)
    {
      setCurrentBlockState(bs_quote);
      length = text.length() - start;
    }
    else
    {
      length = end - start + quoteEndExpression.matchedLength();
    }

    setFormat(start, length, quotationFormat);
    start = quoteStartExpression.indexIn(text, start + length);
  }

  //
  // multi-line comments
  //

  start = -1;
  if (prev == bs_comment)
  {
    start = 0;
  }
  if (prev == bs_none)
  {
    start = commentStartExpression.indexIn(text);
  }

  while (start >= 0)
  {
    int end = commentEndExpression.indexIn(text, start);
    int length;

    if (end == -1)
    {
      setCurrentBlockState(bs_comment);
      length = text.length() - start;
    }
    else
    {
      length = end - start + commentEndExpression.matchedLength();
    }

    setFormat(start, length, singleLineCommentFormat);
    start = commentStartExpression.indexIn(text, start + length);
  }
}