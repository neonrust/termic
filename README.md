# termic

![Output of demo1](screenshots/demo1.png?raw=true "Output of demo1")

TERMinal Interface (using double-buffered grid of) Cells

Inspiration was actually my own, but I later discovered https://github.com/gdamore/tcell and https://github.com/nsf/termbox.

This is a personal project, undergoing lots of changes. It is most
assuredly not ready for any kind of "prime-time".  But feel free to
look around and try it.


## Compatibility

Currently, termic only supports 24-bit color output (without checking
for support).  This might change later, but I don't have a need for
it, personally.

termic also assumes that the terminal supports "standard" escape
sequences (it does not use terminfo):

https://en.wikipedia.org/wiki/ANSI_escape_code


## Dependencies

Some C++20 features might be used somewhere, but C++17 is definitely required.

* libfmt    (actually not, but currently used for debug logging)
* TheWisp signals       (header-only)


## Example

A minimal example of a simple application:

```c++
#include <termic/app.h>

void main()
{
  using namespace termic;

  App app(HideCursor);
  if(not app)
    return 1;

  // listen to key presses
  app.on_key_event.connect([&](const event::Key &k) {
    app.screen().clear();
    app.screen().print(
      { 10, 10 },              // x, y position (zero-based)
      fmt::format("Hello, world!  key: {}", k.to_string())
	);

    // exit the app if escape is pressed
    if(k.key == key::ESCAPE and not k.modifiers)
      app.quit();
  });

  // block until requested to quit
  app.run();
}
```

Possible eutput (pressed key is printed):

![example](screenshots/example-output.png?raw=true "example")
