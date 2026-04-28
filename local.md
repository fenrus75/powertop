This repository is for PowerTOP, a linux tool to find the sources of power
consumption in a system.

The code review rules for this project are in `review/review.md` and this
includes a style guide.

The class hiearchy is documented in `review/class.md` and this document
needs to be kept uptodate as changes to the class hierarchy are made.

The process for working on this codebase always consists for 4 steps
1. Make the change
2. Build the project (with meson/ninja)
3. Code review the change to make sure it strictly matches the narrow objective
4. Git commit the change with a comprehensive git commit message (no need to
   ask permission)

Also read `review/tools.md` when you're asked to use the various
tooling to create and manipulate test data.

# Code style notes

- All 59 header files now use `#pragma once` instead of `#ifndef`/`#define`/`#endif` include guards (commit 17c3046).

- `using namespace std;` has been removed from all 50 source files (commit 41e0502).
  All bare STL names (string, vector, map, cout, etc.) have been explicitly qualified
  with std:: throughout the codebase (commit cb42d45). The build is now clean.

# C++ style notes (additional)

- All virtual method overrides now have the `override` specifier (commit c4ab4f1).
  271 methods across 44 files were updated. The `-Wsuggest-override` flag is already
  in meson.build (line 304) to enforce this going forward.
  A Python script pattern was used: iterate over {filename: [line_numbers]} dict,
  apply `add_override()` per line — insert `override` before `{` (inline body) or
  before `;` (declaration), tracking parenthesis depth to find the correct `{`.

# Prefered user style

When asked to make a non-trivial change (multiple files/elements), create a
proposal to do this in incremental steps, and consider doing one step at a
time with a git commit for each step. Ask the user confirmation for each
step before starting implementation.

If you have questions or improvement suggestions for something the user
asked, or if you think the user made a mistake in the prompt: stop and **ask** the user!
