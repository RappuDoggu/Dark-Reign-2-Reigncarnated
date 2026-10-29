#include "reign/tokenizer.hpp"
#include "reign/string_copy.hpp"
#include <iostream>
#include <stdexcept>
int main() {
 int failures = 0;
 auto check = [&](bool b) { if (!b) ++failures; };
 char overlap[16] = "profile";
 Utils::Strmcpy(overlap, overlap, sizeof overlap);
 check(std::string(overlap) == "profile");
 Utils::Strmcpy(overlap, overlap + 2, sizeof overlap);
 check(std::string(overlap) == "ofile");
 Utils::Strmcpy(overlap + 1, overlap, sizeof overlap - 1);
 check(std::string(overlap + 1) == "ofile");
 Utils::Strmcpy(overlap, "abcdef", 4);
 check(std::string(overlap) == "abc");
 auto parse = [](const std::string &s) { return reign::tokenize(s, "fixture"); };
 auto t = parse("// comment\r\nCall(\"a / b\", '', -12);\r\nNext();");
 check(t.size() == 14);
 check(t[0].text == "Call" && t[0].line == 2);
 check(t[2].kind == reign::TokenKind::string && t[2].text == "a / b");
 check(t[4].kind == reign::TokenKind::string && t[4].text.empty());
 check(t[10].text == "Next" && t[10].line == 3);
 check(parse("").empty());
 check(parse("// only comment").empty());
 check(parse(std::string(255,'a')).size() == 1);
 auto rejects = [&](const std::string &s) { try { parse(s); ++failures; } catch (const std::runtime_error &) {} };
 rejects(std::string(256,'a'));
 rejects("Call(\"unterminated");
 rejects("Call(\"newline\nstring\");");
 std::cout << "Original TBuf checks: " << (failures ? "FAIL" : "PASS") << " (" << failures << " failures)\n";
 return failures ? 1 : 0;
}
