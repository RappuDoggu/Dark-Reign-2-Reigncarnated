#pragma once
#include "reign/edit_support.hpp"
#include "console_slice.hpp"
constexpr S32 DIK_PRIOR=0xC9,DIK_NEXT=0xD1,DIK_L=0x26;
namespace Console {
using VCList=List<VarPathIdent>;using VCIterator=VCList::Iterator;
using HistList=List<char>;using HistIterator=HistList::Iterator;
inline Bool BuildVarCompletionList(const char *,VCList &,VCIterator &,Bool){throw std::runtime_error("Console completion service not bound");}
inline Bool BuildHistoryRecallList(const char *,HistList &,HistIterator &,Bool){throw std::runtime_error("Console history service not bound");}
inline void AddCmdHist(const char *,Bool=TRUE){throw std::runtime_error("Console history service not bound");}
}
