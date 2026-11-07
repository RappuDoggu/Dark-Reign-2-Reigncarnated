#include "reign/tokenizer_prelude.hpp"
#include "tbuf.h"
#include "reign/tokenizer.hpp"
#include <limits>
namespace reign {
std::vector<Token> tokenize(std::string_view text, const std::string &name) {
 if (text.size() > std::numeric_limits<U32>::max()) throw std::runtime_error("Config exceeds original 32-bit size");
 TBuf buf;
 buf.SetCharType("#(,');{}=-+&\"", TBuf::PUNCTUATION);
 buf.SetCharType("/", TBuf::COMMENT);
 buf.Setup(text.empty() ? "" : text.data(), static_cast<U32>(text.size()), name.c_str());
 std::vector<Token> result;
 for (;;) {
  auto kind = buf.NextToken();
  if (kind == TR_EOF) break;
  const auto line = buf.CurrentLine();
  std::string value = buf.lastToken;
  if (kind == TR_PUN && (value == "\"" || value == "'")) {
   const char delimiter = value[0];
   buf.ReadConstant(delimiter);
   kind = buf.NextToken();
   value = buf.lastToken;
   if (kind == TR_PUN && value == std::string(1, delimiter)) value.clear();
   else {
    if (kind != TR_OK) throw std::runtime_error("Invalid string literal");
    const char expected[] = {delimiter, 0};
    buf.Accept(expected);
   }
   result.push_back({TokenKind::string, value, line});
  } else result.push_back({kind == TR_PUN ? TokenKind::punctuation : TokenKind::identifier, value, line});
 }
 return result;
}
}
