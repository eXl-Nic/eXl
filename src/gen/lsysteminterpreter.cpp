#include <gen/lsysteminterpreter.hpp>
#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>
#include <gen/gridrule.hpp>

namespace eXl
{
  void LSystemInterpreter::Rule::Stream(Streamer& iStreamer) const
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Pattern");
    iStreamer.BeginSequence();
    for(auto sym : m_ToMatch)
    {
      unsigned int symVal = sym;
      iStreamer.WriteUInt(&symVal);
    }
    iStreamer.EndSequence();
    iStreamer.PopKey();
    iStreamer.PushKey("Replacement");
    iStreamer.BeginSequence();
    for(auto sym : m_Replacement)
    {
      unsigned int symVal = sym;
      iStreamer.WriteUInt(&symVal);
    }
    iStreamer.EndSequence();
    iStreamer.PopKey();
    iStreamer.EndStruct();
  }

  void LSystemInterpreter::Rule::Unstream(Unstreamer& iUnstreamer)
  {
    m_ToMatch.clear();
    m_Replacement.clear();
    iUnstreamer.BeginStruct();
    iUnstreamer.PushKey("Pattern");
    Err seq1 = iUnstreamer.BeginSequence();
    if((seq1))
    {
      do
      {
        unsigned int symVal;
        iUnstreamer.ReadUInt(&symVal);
        m_ToMatch.push_back((Symbols)symVal);
      }while((seq1 = iUnstreamer.NextSequenceElement()));
    }
    iUnstreamer.PopKey();
    iUnstreamer.PushKey("Replacement");
    seq1 = iUnstreamer.BeginSequence();
    if((seq1))
    {
      do
      {
        unsigned int symVal;
        iUnstreamer.ReadUInt(&symVal);
        m_Replacement.push_back((Symbols)symVal);
      }while((seq1 = iUnstreamer.NextSequenceElement()));
    }
    iUnstreamer.PopKey();
    iUnstreamer.EndStruct();
  }

  void LSystemInterpreter::SanitizeRule(Rule& ioRule)
  {
    Pattern sanitizedPattern;
    bool stopped = false;
    for(auto sym : ioRule.m_ToMatch)
    {
      switch(sym)
      {
      case Forward:
      case Turn90:
      case Turn270:
      case SymA:
      case SymB:
      case SymC:
        sanitizedPattern.push_back(sym);
        break;
      case Stop:
        stopped = true;
        break;
      default:
        break;
      }
      if(stopped)
        break;
    }
    ioRule.m_ToMatch.swap(sanitizedPattern);
    sanitizedPattern.clear();
    stopped = false;

    unsigned int numPushes = 0;
    unsigned int numPop = 0;
    
    std::list<bool> pushStack;

    for(auto sym : ioRule.m_Replacement)
    {
      switch(sym)
      {
      case Push:
        ++numPushes;
        pushStack.push_back(false);
        sanitizedPattern.push_back(sym);
        break;
      case Pop:
        if((numPushes - numPop) > 0)
        {
          if(!pushStack.back())
          {
            for(int i = sanitizedPattern.size() - 1; i>= 0; --i)
            {
              if(sanitizedPattern[i] == Push)
              {
                sanitizedPattern.erase(sanitizedPattern.begin() + i);
                break;
              }
            }
            --numPushes;
          }
          else
          {
            ++numPop;
            sanitizedPattern.push_back(sym);
          }
          pushStack.pop_back();
        }
        break;
      case Stop:
        stopped = true;
        break;
      default:
        if(!pushStack.empty())
          pushStack.back() = true;
        sanitizedPattern.push_back(sym);
        break;
      }
      if(stopped)
        break;
    }

    for(unsigned int i = 0; i< (numPushes - numPop); ++i)
    {
      sanitizedPattern.push_back(Pop);
    }

    ioRule.m_Replacement.swap(sanitizedPattern);
    return;
  }

  void LSystemInterpreter::ApplyRules(Vector<Rule> const& iRules, Vector<Symbols>& ioString)
  {
    if(ioString.size() > 0)
    {
      Multimap<unsigned int, unsigned int> sortedRules;

      for(unsigned int i = 0; i<iRules.size(); ++i)
      {
        auto const& rule = iRules[i];
        if(rule.m_ToMatch.size() > 0)
        {
          sortedRules.insert(std::make_pair(rule.m_ToMatch.size(), i));
        }
      }

      unsigned int longestPattern = sortedRules.rbegin()->first;

      Vector<Symbols> newStr;
      unsigned int currentIdx = 0;
      while(currentIdx < ioString.size())
      {
        bool replacementDone = false;;
        unsigned int maxPattern = Mathi::Clamp(ioString.size() - currentIdx, 0, longestPattern);
        for(auto iter = sortedRules.rbegin(); iter!=sortedRules.rend() && !replacementDone; ++iter)
        {
          auto const& rule = iRules[iter->second];
          if(iter->first <= maxPattern
          && memcmp(&ioString[currentIdx], &rule.m_ToMatch[0], iter->first * sizeof(Symbols)) == 0)
          {
            replacementDone = true;
            currentIdx += iter->first;
            newStr.insert(newStr.end(), rule.m_Replacement.begin(), rule.m_Replacement.end());
          }
        }
        if(!replacementDone)
        {
          newStr.push_back(ioString[currentIdx]);
          ++currentIdx;
        }
      }
      ioString.swap(newStr);
    }
  }
  /*
  LevelGrammar_Old* LSystemInterpreter::MakeGrammar(Vector<Rule> const& iRules, unsigned int& oNumErrors)
  {
    oNumErrors = 0;
    bool oneRuleIsValid = false;
    Vector<int> hasValidSymbol(iRules.size(), -1);
    for(unsigned int curRule = 0; curRule < iRules.size(); ++curRule)
    {
      Pattern const& toMatch = iRules[curRule].m_ToMatch;
      for(unsigned int i = 0; i<toMatch.size(); ++i)
      {
        bool hasStopped = false;
        auto sym = toMatch[i];
        switch(sym)
        {
        case Push:
        case Pop:
          ++oNumErrors;
          break;
        case Stop:
          hasStopped = true;
          break;
        default:
          oneRuleIsValid = true;
          hasValidSymbol[curRule] = i;
          break;
        }
        if(hasStopped)
          break;
      }
    }
    if(oneRuleIsValid)
    {
      static const String names[9] = 
      {
        EXL_TEXT("Forward") ,
        EXL_TEXT("Push") ,
        EXL_TEXT("Pop")  ,
        EXL_TEXT("Turn90") ,
        EXL_TEXT("Turn270") ,
        EXL_TEXT("SymA") ,
        EXL_TEXT("SymB") ,
        EXL_TEXT("SymC") ,
        EXL_TEXT("Stop")
      };
      LevelGrammar_Old* newGrammar = new LevelGrammar_Old(8, names);
      
      for(unsigned int curRule = 0; curRule < iRules.size(); ++curRule)
      {
        LevelGrammar_Old::RuleBuilder builder = newGrammar->StartRule();
        if(hasValidSymbol[curRule] >= 0)
        {
          int prevIdx = -1;
          Pattern const& toMatch = iRules[curRule].m_ToMatch;
          for(unsigned int i = 0; i<toMatch.size(); ++i)
          {
            auto sym = toMatch[i];
            bool hasStopped = false;
            switch(sym)
            {
            case Push:
            case Pop:
              break;
            case Stop:
              hasStopped = true;
              break;
            default:
              builder.AddNode(sym, i != 0, i != hasValidSymbol[curRule]); 
              if(prevIdx >= 0)
              {
                builder.AddConnection(prevIdx, prevIdx +1);
              }
              ++prevIdx;
              break;
            }
            if(hasStopped)
              break;
          }
          unsigned int matchSize = prevIdx + 1;
          builder.EndMatch();
          builder.BeginRepl();
          builder.InNodes(0);
          prevIdx = 0;
          List<unsigned int> stack;
          unsigned int lastNodeId = 0;
          for(auto sym : iRules[curRule].m_Replacement)
          {
            bool hasStopped = false;
            switch(sym)
            {
            case Push:
              stack.push_back(lastNodeId);
              break;
            case Pop:
              if(!stack.empty())
              {
                prevIdx = stack.back();
                stack.pop_back();
              }
              else
                ++oNumErrors;
              break;
            case Stop:
              hasStopped = true;
              break;
            default:
              builder.AddNode(sym);
              builder.AddConnection(prevIdx, lastNodeId + 1);
              prevIdx = lastNodeId + 1;
              ++lastNodeId;
              break;
            }
            if(hasStopped)
              break;
          }
          
          builder.OutNodes(matchSize - 1);
          while(!stack.empty())
          {
            ++oNumErrors;
            prevIdx = stack.back();
            stack.pop_back();
          }

          builder.AddConnection(prevIdx, lastNodeId + 1);

          builder.EndRepl();
          builder.End();
        }
      }

      return newGrammar;
    }
    return NULL;
  }
  */
  void LSystemInterpreter::FillMoves(LSystemInterpreter::Pattern const& iString, Vector<Item>& oMoves)
  {
    for(auto sym : iString)
    {
      Item newItem;
      switch(sym)
      {
      case LSystemInterpreter::SymA:
      case LSystemInterpreter::SymB:
      case LSystemInterpreter::SymC:
        break;
      case LSystemInterpreter::Forward:
        newItem.m_Action = TurtleGrammar::Advance;
        newItem.m_ParamValue = 0;
        oMoves.push_back(newItem);
        break;
      case LSystemInterpreter::Turn90:
        newItem.m_Action = TurtleGrammar::Turn;
        newItem.m_ParamValue = 1;
        oMoves.push_back(newItem);
        break;
      case LSystemInterpreter::Turn270:
        newItem.m_Action = TurtleGrammar::Turn;
        newItem.m_ParamValue = 3;
        oMoves.push_back(newItem);
        break;
      case LSystemInterpreter::Push:
        newItem.m_Action = TurtleGrammar::Push;
        oMoves.push_back(newItem);
        break;
      case LSystemInterpreter::Pop:
        newItem.m_Action = TurtleGrammar::Pop;
        oMoves.push_back(newItem);
        break;
      }
    }
  }

}
