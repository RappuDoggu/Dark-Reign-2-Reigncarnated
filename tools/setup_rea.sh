#!/usr/bin/env bash
# Original Reigncarnated tooling. Linux x86_64 only.
set -euo pipefail
if [[ $# != 1 ]]; then echo "Usage: bash tools/setup_rea.sh /absolute/new/toolchain-directory" >&2; exit 2; fi
prefix=$1
[[ $prefix = /* && ! -e $prefix ]] || { echo "Use an absolute, nonexistent directory; existing installations are preserved." >&2; exit 2; }
[[ $(uname -s) = Linux && $(uname -m) = x86_64 ]] || { echo "This recipe supports Linux x86_64 only." >&2; exit 2; }
for tool in node npm curl sha256sum tar unzip ps; do command -v "$tool" >/dev/null; done
node --input-type=module -e '
const [a,b,c]=process.versions.node.split(".").map(Number);
if (!((a===22&&(b>23||(b===23&&c>=4)))||(a===24&&(b>22||(b===22&&c>=1)))||a>26||(a===26&&(b>12||(b===12&&c>=1))))) {
 console.error("REA requires Node ^22.23.4 || ^24.22.1 || >=26.12.1"); process.exit(2);
}'
mkdir -p "$prefix/downloads" "$prefix/jdk" "$prefix/logs"
fetch() {
 local url=$1 dest=$2 digest=$3
 if [[ -n ${REIGN_REA_DOWNLOAD_CACHE:-} && -f "$REIGN_REA_DOWNLOAD_CACHE/${url##*/}" ]]; then
  cp --reflink=auto "$REIGN_REA_DOWNLOAD_CACHE/${url##*/}" "$dest"
 else
  curl --fail --location --proto '=https' --proto-redir '=https' --retry 3 "$url" -o "$dest"
 fi
 printf '%s  %s\n' "$digest" "$dest" | sha256sum -c -
}
fetch 'https://github.com/adoptium/temurin21-binaries/releases/download/jdk-21.0.13%2B9/OpenJDK21U-jdk_x64_linux_hotspot_21.0.13_9.tar.gz' "$prefix/downloads/jdk.tar.gz" d7f5d5cf865a2902b70ff549b97586a7d3f234e016f041ab447ea740c7d66d1c
tar -xzf "$prefix/downloads/jdk.tar.gz" --strip-components=1 -C "$prefix/jdk"
fetch 'https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.1.4_build/ghidra_12.1.4_PUBLIC_20260921.zip' "$prefix/downloads/ghidra.zip" ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db
unzip -q "$prefix/downloads/ghidra.zip" -d "$prefix"
# npm registry resolution/integrity is recorded by package-lock.
npm install --prefix "$prefix/cli" --save-exact rea-agents@6.8.0 >"$prefix/logs/npm-install.log" 2>&1
export JAVA_HOME="$prefix/jdk"
export GHIDRA_INSTALL_DIR="$prefix/ghidra_12.1.4_PUBLIC"
export PATH="$JAVA_HOME/bin:$prefix/cli/node_modules/.bin:$PATH"
# Generate sourceable env without changing shell profiles or MCP registrations.
printf 'export JAVA_HOME=%q\nexport GHIDRA_INSTALL_DIR=%q\nexport PATH=%q:"$PATH"\n' "$JAVA_HOME" "$GHIDRA_INSTALL_DIR" "$JAVA_HOME/bin:$prefix/cli/node_modules/.bin" >"$prefix/env.sh"
node --version >"$prefix/logs/node-version.txt"
java -version >"$prefix/logs/java-version.txt" 2>&1
javac -version >"$prefix/logs/javac-version.txt" 2>&1
rea --version >"$prefix/logs/rea-version.txt" 2>&1
rea doctor --provider ghidra --json >"$prefix/logs/doctor.json" 2>"$prefix/logs/doctor.stderr"
rea providers --json >"$prefix/logs/providers.json" 2>"$prefix/logs/providers.stderr"
echo "Readiness commands completed. Inspect provider output; decompilation is a separate verification."
