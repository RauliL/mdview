# mdview

GTK application which does nothing but renders [Markdown] document onto the
screen, using WebKit.

## Requirements

- [GTKmm] 3.0
- [WebKitGTK2] 4.0

## How to build

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

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
