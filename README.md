# mdview

GTK application which does nothing but renders [Markdown] document onto the
screen, using WebKit.

## Requirements

- [GTKmm] 3.0
- [WebKitGTK2] 4.0
- [hoedown] (included in the repository)

## How to build

Before you can compile the executable, you need to update dependencies included
in the repository as Git submodules. You can do this with:

```bash
$ git submodule update --init
```

After this you need to install other requirements, as well as C++ compiler and
[CMake] if you do not have them installed already. On Ubuntu systems this seems
to do the trick:

```bash
$ sudo apt install build-essential cmake libwebkit2gtk-4.0-dev libgtkmm-3.0-dev
```

Then you can proceed to compile the actual executable. This can be done with:

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

And you should have `mdview` binary available in the `build` directory.

## How to install

After the executable has been compiled, you can install it to your system like
this:

```bash
$ sudo make install
```

## How to use

```bash
$ mdview filename.md
```

[Markdown]: https://en.wikipedia.org/wiki/Markdown
[GTKmm]: https://gtkmm.org
[WebKitGTK2]: https://webkitgtk.org
[hoedown]: https://github.com/hoedown/hoedown
[CMake]: https://cmake.org
