This repository is for PowerTOP, a linux tool to find the sources of power
consumption in a system.

Do **NOT** update this file to record the results of code review or fixes
for code review comments!

The code review rules for this project are in `review/review.md` and this
includes a style guide.

The class hiearchy is documented in `review/class.md` and this document
needs to be kept uptodate as changes to the class hierarchy are made.

The process for working on this codebase always consists of 5 steps
1. Make the change
2. Build the project (with meson/ninja)
   - Use `ninja -C <builddir> -t clean && ninja -C <builddir>` when the build cache seems stale
   - The build output goes through a pipe in many invocations; "no work to do" after a clean means the build already completed in the same shell pipeline
3. Run the test suite: `ninja -C <builddir> test` (requires `-Denable-tests=true` at setup time)
   - When adding new tests, read `tests/testdesign.md` for conventions and patterns.
4. Code review the change to make sure it strictly matches the narrow objective
5. Git commit the change with a comprehensive git commit message (no need to
   ask permission)

Also read `review/tools.md` when you're asked to use the various
tooling to create and manipulate test data.

# [[maybe_unused]] placement rule

`[[maybe_unused]]` must appear **before** the full parameter declaration,
not after a `*` or `&` qualifier or between type and name.

Correct:   `[[maybe_unused]] struct foo *name`
Correct:   `[[maybe_unused]] const std::string &name`
Correct:   `[[maybe_unused]] int name`
Wrong:     `struct foo *[[maybe_unused]] name`
Wrong:     `const std::string &[[maybe_unused]] name`
Wrong:     `int [[maybe_unused]] name`

GCC warnings triggered by the wrong placement:
- `'maybe_unused' on a type other than class or enumeration definition`
- `attribute ignored`
- `unused parameter`

# Git commit notes

Git commits in this repo must use `--no-gpg-sign` flag to avoid hanging due
to GPG agent unavailability:
```
git commit --no-gpg-sign -F /tmp/commitmsg.txt
```
The repo has a global `user.signingkey` set, but the GPG agent is not running
in the terminal session. Setting `commit.gpgsign=false` locally is not
sufficient — `--no-gpg-sign` on the command line is required.

# Prefered user style

When asked to make a non-trivial change (multiple files/elements), create a
proposal to do this in incremental steps, and consider doing one step at a
time with a git commit for each step. Ask the user confirmation for each
step before starting implementation.

If you have questions or improvement suggestions for something the user
asked, or if you think the user made a mistake in the prompt: stop and **ask** the user!
