#!/bin/sh
# Regenerate compile_commands.json for this Make-based C project.
#
# A clean build is essential: bear/compiledb only record translation
# units that actually compile during the run. If the tree is already
# built, `make` says "nothing to do" and the database comes out empty
# or partial. So we always `make clean` first.
set -eu

root=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
cd "$root"

echo "Cleaning previous build so every translation unit is recorded..."
make clean >/dev/null 2>&1 || true

if command -v bear >/dev/null 2>&1; then
  echo "Generating compile_commands.json with bear..."
  bear -- make
elif command -v compiledb >/dev/null 2>&1; then
  echo "bear not found; falling back to compiledb..."
  compiledb make
else
  echo "error: neither 'bear' nor 'compiledb' is installed." >&2
  echo "Install one, e.g.:  sudo apt-get install -y bear   (or: pipx install compiledb)" >&2
  exit 1
fi

if [ ! -s compile_commands.json ]; then
  echo "error: compile_commands.json was not produced." >&2
  exit 1
fi

n=$(grep -c '"file"' compile_commands.json 2>/dev/null || echo '?')
echo
echo "Done: $root/compile_commands.json ($n entries)."
echo "Restart your editor's clangd / language server to pick up the changes."
