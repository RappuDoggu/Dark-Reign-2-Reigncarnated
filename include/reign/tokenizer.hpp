#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace reign {
enum class TokenKind { identifier, punctuation, string };
struct Token { TokenKind kind; std::string text; std::uint32_t line; };
// Calls the ORIGINAL TBuf implementation, not a replacement grammar/parser.
// Quoted literals are grouped using its ReadConstant/NextToken operations.
std::vector<Token> tokenize(std::string_view text, const std::string &name);
}
