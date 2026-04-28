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

- [ ] All user visible strings must be translatable and use the `_()` gettext
    pattern.

- [ ] When formatting a translated string that contains `{}` placeholders, you
    **must** use `pt_format` instead of `std::format`. `std::format` requires a
    compile-time format string; `_()` returns a runtime `const char*` from
    gettext, so `std::format(_("..."), x)` will not compile. `pt_format` wraps
    `std::vformat` which accepts runtime strings.

    **Use `std::format` when the string is not translated** (internal/technical
    strings, data-derived identifiers, log output not shown to the user):
    ```cpp
    // OK — not user-visible, no translation needed
    std::string path = std::format("{}/actual_brightness", sysfs_path);
    ```

    **Use `pt_format(_(...))` when the string is user-visible and needs
    translation:**
    ```cpp
    // OK — user-visible label with a runtime argument
    humanname = pt_format(_("USB device: {}"), product);
    ```

- [ ] malloc() does not fail in practice. Checking for failure is not a coding style
    violation, but the lack of checking is at most of `nit` severity.