# termic

![demo1](screenshots/demo1.png?raw=true "demo1")

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


## Example

```
#include "app.h"

void main()
{
  using namespace termic;

  App app(Fullscreen | HideCursor | MouseEvents);
  if(not app)
    return 1;

  app.on_key_event.connect([&app](const event::Key &k) {
    app.screen().clear();
    app.screen().print({ 10, 10 }, fmt::format("Hello, world!  key: {}", key::to_string(k.key, k.modifiers)), color::White, color::rgb(30, 120, 20), style::Bold);

    if(k.key == key::ESCAPE and k.modifiers == key::NoMod)
      app.quit();
  });

  app.run();
}
```

Output:

![example](screenshots/example-output.png?raw=true "example")
