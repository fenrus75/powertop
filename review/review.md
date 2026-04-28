# PowerTOP review guidelines

PowerTOP is a Linux CLI application used to diagnose software that is
causing high power usage, and to suggest tunings on the system for best
power.


## Review Process

A three step process
1. Gather context
2. Perform code review
3. Report results


### Gather context

When doing code review, always gather the whole function under review,
even if a PR or patch only touches a few lines.
For patch/PR reviews, collect both the original and the patched full
function.

In addition, load these files into the context:
- `style.md` - Coding style guide
- `general-c++.md` - C++ coding rules
- `rules.md` - PowerTOP specific coding rules


## Perform code review

Perform your normal code review process, but also check coding style
(`style.md`), and **in addition** use the other rules that you collected in the 
"Gather context" phase as part of your code review.

Assign a priority to each issue found: 'nit', 'low', 'medium', 'high',
'critical'.


## Reporting results
Generate a report in Markdown format. Use `review.md` as filename unless the
prompt requested a different filename or filenames.

Template output format:
```
# PowerTOP review of <filename, git commit hash or PR number>

<summary of the prompt/task>

## Critical items

### Critical item #1 : <short description>

Location:  <filename> : <line number>
Description: <one or two paragraph description>
Severity rationale: <why was this critical>
Suggested fix: <proposed fix, if any. leave out if none>

## High severity items

### High severity item #1 : <short description>
...
## Medium severity items

## Low severity items

## Nit severity items

# Tests performed

<a list of all checks performed during this code review>
```
This template shows only 1 critical item for the purpose of illustration, but use the same format for any
item found at any severity.

Leave out any severity levels for which nothing was found. 

