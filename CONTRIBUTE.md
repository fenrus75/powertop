# How to Contribute to PowerTOP

If you are interested in PowerTOP and wish to put some time and effort into it,
this is the document for you. Filing bug reports, translating text strings,
editing or creating documentation, helping fellow users, or making changes to
the program source code are some of the ways you can contribute to the project.

Thank you for your interest in PowerTOP. We appreciate your effort, time, and
continued participation in this project.


# Official source code repository

The official source code repository is hosted on GitHub*, so a GitHub account is
strongly recommended for effective collaboration. All bug filings, changes, and
releases are hosted there:
* [Official Upstream Repository][upstream]

Developers, check out the "Code Conventions" section further down. We look
forward to your pull requests!


# Finding bugs

No software is perfect. Since PowerTOP is a diagnostic/debug tool concerned
with your system, different hardware may behave differently than we expect,
resulting in weird behaviors (or crashes). There are plenty of bugs to be found
and squashed, and we appreciate your willingness to help.

Bugs will be filed here:
* [Official Upstream Repository][upstream]

It is important to note that bug filings are the start of a conversation, and
conversations are two-way streets. When you find a bug, do not merely "file and
forget". Expect follow-up questions to be asked. We may even ask you to patch,
re-compile, and re-run to see if our change(s) fix your issue, especially if
your system's particular behavior(s) have revealed the bug.

If one of us asks you to do something that tests the limits of your comfort or
ability, say so, and we will do our best to help you.


# Writing documentation

Here is the layout of PowerTOP's documentation:
* `CONTRIBUTE.md` is this document
* `README.md` covers cloning, building, and getting started with PowerTOP
* `doc/powertop.8` is the (roff) manual page for the project
* Code comments (or lack thereof)

PowerTOP, at present, does not generate a library for use by other programs, so
there is no "PowerTOP API". The source code does have internal functions and
APIs for specific duties that are key to program functionality. Those need to
be thoroughly documented for future developers.

Markdown documents follow the latest CommonMark spec. Generally speaking,
do not use GitHub's proprietary extensions to Markdown syntax.
* [CommonMark Specifications][cmark]

The manual page uses roff syntax.
* `man 7 roff`, or
* [man 7 roff (in html)][manroff]

Code comments are described in the "Code Conventions" section further down.


# Localization

PowerTOP uses `gettext` for localization. Translating strings is done using PO
files, and those are contributed back to PowerTOP through a GitHub pull
request.

If you are new to `gettext`, here is the official documentation for it:
* [Creating PO Files][gettext_create]

The `gettext` project also has some links to commonly-used PO editors:
* [Editing PO Files][gettext_edit]


# Code conventions

PowerTOP's maintainers will enforce the Linux* kernel coding style.
* [Linux Kernel Coding Style][kernel_style]

That style guide is tailored for the Linux kernel, so "I" is Linus Torvalds.
For the purposes of the PowerTOP project, the maintainers of PowerTOP have the
final say on style. If a specific thing does not apply to PowerTOP, the essence
of the guidance almost surely does. This section will be expanded upon
as needed.

While C++ is used in some parts of the codebase, C-style comments are
preferred. This is copy-pasted directly from the kernel style guide:

    /*
     * This is the preferred style for multi-line
     * comments in the Linux kernel source code.
     * Please use it consistently.
     *
     * Description:  A column of asterisks on the left side,
     * with beginning and ending almost-blank lines.
     */

There are sections of that style guide that are specific to the kernel-- the
"Allocating Memory" and "Don't re-invent the kernel macros" sections come to
mind. The essential take-aways from those sections are: "Use the right function
for the right job" and "do not re-invent existing functionality". That is sound
development advice for any software project.

The PowerTOP code base is constantly evolving, and the code base has been
around for a while. Expect to see code that does not follow the kernel style
guide. If you find any such code, fix it or let us know. Those are style bugs
that need fixing.

When you submit a patch to the PowerTOP project, we assume you agree to
what's written in the "License" section below.


# License

PowerTOP is a GPLv2 project (see `COPYING` and `README.md`). All contributions
will be licensed under the terms of the GPLv2.

If you create a new source file or header, here is the comment to be placed at
the beginning (from line 1) of the new file:

```
/*
 * Copyright (C) yyyy  name of author
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
```

All contributions to PowerTOP *must* adhere to the Linux kernel's "Developer's
Certificate of Origin", which we will copy-paste here for completeness:

By making a contribution to this project, you certify that:

```
a. The contribution was created in whole or in part by me and I have the right
   to submit it under the open source license indicated in the file; or

b. The contribution is based upon previous work that, to the best of my
   knowledge, is covered under an appropriate open source license and I have the
   right under that license to submit that work with modifications, whether
   created in whole or in part by me, under the same open source license (unless I
   am permitted to submit under a different license), as indicated in the file; or

c. The contribution was provided directly to me by some other person who
   certified (a), (b) or (c) and I have not modified it.

d. I understand and agree that this project and the contribution are public and
   that a record of the contribution (including all personal information I submit
   with it, including my sign-off) is maintained indefinitely and may be
   redistributed consistent with this project or the open source license(s)
   involved.
```

From: [Developer's Cerificate of Origin][kernel_cos]


# Thank You!

Again, thank you! Your time and effort are greatly appreciated! We look forward
to your contributions, and appreciate your continued participation in the
project!


[upstream]: https://github.com/fenrus75/powertop "Official PowerTOP Repository"
[cmark]: https://spec.commonmark.org/ "External Link: CommonMark Specifications"
[manroff]: http://man7.org/linux/man-pages/man7/roff.7.html "External Link: man 7 roff"
[gettext_create]: https://www.gnu.org/software/gettext/manual/html_node/Creating.html#Creating "External Link: Creating a PO File"
[gettext_edit]: https://www.gnu.org/software/gettext/manual/html_node/Editing.html#Editing "External Link: PO File Editors"
[kernel_style]: https://www.kernel.org/doc/html/latest/process/coding-style.html "External Link: Linux kernel coding style"
[kernel_cos]: https://www.kernel.org/doc/html/latest/process/submitting-patches.html#sign-your-work-the-developer-s-certificate-of-origin "External Link: Developer's Certificate of Origin"
