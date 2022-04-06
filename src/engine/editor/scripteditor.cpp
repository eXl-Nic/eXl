#include "scripteditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"

#include "collectionmodel.hpp"

#include <core/type/typemanager.hpp>

#include <engine/game/commondef.hpp>
#include <engine/script/luascriptbehaviour.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextEdit>

class QTextDocument;

class LuaHighlighter : public QSyntaxHighlighter
{
public:
  LuaHighlighter(QTextDocument *parent = 0);

protected:
  void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

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

namespace eXl
{
  
  ResourceEditorHandler& LuaScriptEditor::GetEditorHandler()
  {
    static EditorHandler_T<LuaScriptBehaviour, LuaScriptEditor> s_Handler;
    return s_Handler;
  }

  struct LuaScriptEditor::Impl
  {
    Impl()
    {
    }

    void GenerateDefaultBehaviourSource();

    LuaScriptEditor* m_Editor;
    LuaScriptBehaviour* m_Script;

    QComboBox* m_BehaviourSelector;
    QTextEdit* m_ScriptSrc;
    Vector<String> m_Behaviours;
  };

  void LuaScriptEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  LuaScriptEditor::LuaScriptEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl)
  {
    m_Impl->m_Editor = this;
    m_Impl->m_Script = LuaScriptBehaviour::DynamicCast(iDoc->GetResource());

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_Impl->m_BehaviourSelector = new QComboBox(this);
    EventsManifest const& manifest = EngineCommon::GetBaseEvents();
    m_Impl->m_BehaviourSelector->addItem("<empty>");
    for (auto const& entry : manifest.m_Interfaces)
    {
      m_Impl->m_Behaviours.push_back(entry.first);      
      m_Impl->m_BehaviourSelector->addItem(QString::fromUtf8(entry.first.c_str()));
    }

    auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
    if (iter != manifest.m_Interfaces.end())
    {
      auto iter = std::find(m_Impl->m_Behaviours.begin(), m_Impl->m_Behaviours.end(), m_Impl->m_Script->m_InterfaceName);
      uint32_t index = iter - m_Impl->m_Behaviours.begin();
      m_Impl->m_BehaviourSelector->setCurrentIndex(index + 1);
    }

    QObject::connect(m_Impl->m_BehaviourSelector, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged,
      [this](int iIndex)
    { 
      if (iIndex == 0)
      {
        EventsManifest const& manifest = EngineCommon::GetBaseEvents();
        auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
        if (iter == manifest.m_Interfaces.end())
        {
          return;
        }
        m_Impl->m_Script->m_InterfaceName = "";
        m_Impl->m_Editor->ModifyResource();
      }
      else
      {
        String itfName = m_Impl->m_Behaviours[iIndex - 1];
        if (itfName == m_Impl->m_Script->m_InterfaceName)
        {
          return;
        }
        m_Impl->m_Script->m_InterfaceName = itfName;
        m_Impl->m_Editor->ModifyResource();

        if (m_Impl->m_Script->m_Script.empty())
        {
          m_Impl->GenerateDefaultBehaviourSource();
        }
      }
    });

    layout->addWidget(m_Impl->m_BehaviourSelector); 

    m_Impl->m_ScriptSrc = new QTextEdit(this);
    m_Impl->m_ScriptSrc->setTabStopWidth(m_Impl->m_ScriptSrc->tabStopWidth() / 2);
    new LuaHighlighter(m_Impl->m_ScriptSrc->document());
    m_Impl->m_ScriptSrc->setText(QString::fromUtf8(m_Impl->m_Script->m_Script.c_str()));

    layout->addWidget(m_Impl->m_ScriptSrc);

    QObject::connect(m_Impl->m_ScriptSrc, &QTextEdit::textChanged, [this]
    {
      m_Impl->m_Script->m_Script = m_Impl->m_ScriptSrc->toPlainText().toUtf8().data();
      m_Impl->m_Editor->ModifyResource();
    });

    setLayout(layout);
  }

  void LuaScriptEditor::Impl::GenerateDefaultBehaviourSource()
  {
    EventsManifest const& manifest = EngineCommon::GetBaseEvents();
    auto iter = manifest.m_Interfaces.find(m_Script->m_InterfaceName);
    if (iter != manifest.m_Interfaces.end())
    {
      return;
    }
    String scriptObjName = m_Script->GetName();
    scriptObjName.append("_");
    scriptObjName.append(iter->first);
    scriptObjName.append("_script");
    
    String defaultScript;
    defaultScript.append("local module ");
    defaultScript.append(scriptObjName);
    defaultScript.append(" = {}\n\n");
    
    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Init(object)\n\nend\n\n");

    for (auto const funDesc : iter->second)
    {
      defaultScript.append("function ");
      defaultScript.append(scriptObjName);
      defaultScript.append(".");
      defaultScript.append(funDesc.first);
      defaultScript.append("(scriptObj");

      for (uint32_t i = 0; i<funDesc.second.arguments.size(); ++i)
      {
        Type const* type = funDesc.second.arguments[i];
        defaultScript.append(", ");
        defaultScript.append(type->GetName());
        defaultScript.append("_arg");
        defaultScript.append(StringUtil::FromInt(i));
      }
      defaultScript.append(")\n");
      if (funDesc.second.returnType != nullptr)
      {
        defaultScript.append("--return ");
        defaultScript.append(funDesc.second.returnType->GetName());
        defaultScript.append("()\n");
      }
      defaultScript.append("end\n\n");
    }
    defaultScript.append("return ");
    defaultScript.append(scriptObjName);
    defaultScript.append("\n");

    m_Script->m_Script = std::move(defaultScript);
    m_ScriptSrc->setText(QString::fromUtf8(m_Script->m_Script.c_str()));
  }
}