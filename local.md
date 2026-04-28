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

- `using namespace std;` has been removed from all 50 source files (commit 41e0502).
  All bare STL names (string, vector, map, cout, etc.) have been explicitly qualified
  with std:: throughout the codebase (commit cb42d45). The build is now clean.

# Prefered user style

When asked to make a non-trivial change (multiple files/elements), create a
proposal to do this in incremental steps, and consider doing one step at a
time with a git commit for each step. Ask the user confirmation for each
step before starting implementation.

If you have questions or improvement suggestions for something the user
asked, or if you think the user made a mistake in the prompt: stop and **ask** the user!
