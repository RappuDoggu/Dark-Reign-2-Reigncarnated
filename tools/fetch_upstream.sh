#!/usr/bin/env bash
set -euo pipefail
# This script downloads into a separate user-selected path, never into our source tree.
: "${1:?usage: bash tools/fetch_upstream.sh /absolute/temporary/checkout}"
git clone https://github.com/grasmanek94/darkreign2.git "$1"
git -C "$1" checkout --detach 9ce9da5f21604310a997f0c41e9cd383f5e292c3
