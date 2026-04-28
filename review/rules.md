# PowerTOP code review checks



## Directory handling

- [ ] When using opendir/readdir or equivalent, the code must skip the "."
    and ".." entries in the directory and not process them as normal


## C++ style

- [ ] All virtual method overrides in derived classes must use the `override`
    specifier. The `virtual` keyword may be kept or omitted on overrides, but
    `override` is mandatory. Pure virtual declarations in base classes (`= 0`)
    do not need `override`. The build is configured with `-Wsuggest-override`
    to enforce this.

- [ ] Use `nullptr` instead of `NULL` in all C++ code (`.cpp` and `.h` files).


## General checks

- [ ] All user visible strings must be translatable and use the _() gettext
    pattern

- [ ] malloc() does not fail in practice. Checking for failure is not a coding style
    violation, but the lack of checking is at most of `nit` severity.