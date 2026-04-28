# PowerTOP code review checks



## Directory handling

- [ ] When using opendir/readdir or equivalent, the code must skip the "."
    and ".." entries in the directory and not process them as normal


## General checks

- [ ] All user visible strings must be translatable and use the _() gettext
    pattern