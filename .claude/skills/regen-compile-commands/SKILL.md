---
name: regen-compile-commands
description: Regenerate this project's compile_commands.json (the clangd/LSP compilation database) with bear. Use this whenever compile_commands.json is stale or missing, after editing the makefiles or compiler flags, after adding or removing source files, or when the editor/clangd reports errors that a clean command-line `make` does not — e.g. phantom "cannot combine type", redefinition, or unknown-symbol errors that disappear when you compile the same file directly. Reach for this anytime the IDE's C IntelliSense is out of sync with the real build.
---

# Regenerate compile_commands.json

`compile_commands.json` is the compilation database clangd (and other
C tooling) reads to know exactly how each source file is compiled —
include paths, `-D` defines, warning flags, language standard. When it
drifts from the actual build, the editor shows errors that don't exist
in a real `make`, or misses new files. This regenerates it from a real
build of the current tree.

## How to do it

Run the bundled script from anywhere inside the repo:

```sh
sh .claude/skills/regen-compile-commands/scripts/regen.sh
```

It cleans the build, runs `bear -- make` to capture the real compiler
invocations, writes `compile_commands.json` at the repo root, and
reports the entry count.

## Why a clean build first

`bear` (and `compiledb`) only record translation units that *actually
compile* during the run — they observe the compiler being exec'd. If
the tree is already built, `make` prints "nothing to do", no compiler
runs, and you get an empty or partial database. The script always
`make clean`s first for this reason; if you regenerate by hand, do the
same: `make clean && bear -- make`.

## After regenerating

The database is refreshed, but a running clangd has already cached the
old one. Tell the user to **restart their editor's language server**
(e.g. "clangd: Restart language server" in VS Code, or reload the
window) — you can't do this for them. That's what clears stale errors
like the `size_t`/`bool` "cannot combine" messages.

## Notes

- `compile_commands.json` is gitignored here, so regenerating it
  produces no repository change to commit — it's purely a local tooling
  artifact.
- Fallback: if `bear` isn't installed the script uses `compiledb make`
  instead. If neither is present it tells you how to install one
  (`sudo apt-get install -y bear`, or `pipx install compiledb`).
- The database captures the default (release) build flags. The
  `DEBUG=y` build uses different optimization flags but the same
  includes/defines, so it doesn't need its own database for clangd.
