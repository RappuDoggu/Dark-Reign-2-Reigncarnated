#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>
namespace reign {
using ConfigValue=std::variant<std::int32_t,float,std::string>;
struct ConfigNode {
 std::string name;
 std::uint32_t crc;
 std::vector<ConfigValue> arguments;
 std::vector<ConfigNode> children;
};
using ConfigLoader=std::function<std::vector<std::uint8_t>(const std::string &)>;
// Parses through the original PTree/FScope/VNode/StdParse/TBuf implementations.
// Export is a diagnostic snapshot of function scopes, not an alternate interpreter.
std::vector<ConfigNode> parse_config(ConfigLoader,const std::string &filename);
// Original PTree writers exercised for layout/parity tests. Output paths are caller-controlled.
void roundtrip_config(ConfigLoader,const std::string &filename,const std::string &text_output,const std::string &binary_output);
}
