# termic

![demo1](screenshots/Screenshot_2021-12-21.png?raw=true "demo1")

TERMinal Interface (using) Cells

Inspiration was actually my own, but I later discovered https://github.com/gdamore/tcell and https://github.com/nsf/termbox.

This is a personal project, undergoing lots of changes. It is most
assuredly not ready for any kind of "prime-time".  But feel free to
look around and try it.


## Compatibility

Currently, termic only supports 24-bit color output (without checking
for support).  This might change later, but I don't have a need for
it, personally.

termic also assumes that the terminal supports "standard" escape
sequences. It does not use the terminfo
database. https://en.wikipedia.org/wiki/ANSI_escape_code was used as
reference.


## Dependencies

Some C++20 features might be used somewhere, but C++17 is definitely required.

* libfmt  (actually not, but currently used for debug logging)
* TheWhisp signals  (vendored, header-only)
