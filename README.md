# derzvim
A minimal Vim clone just for fun

## Dependencies
[Lua 5.1](http://www.lua.org/home.html) (MIT) - A powerful, efficient, lightweight, embeddable scripting language

## Building
This project is built using POSIX-compatible [make](https://pubs.opengroup.org/onlinepubs/009695399/utilities/make.html).
For unix-like systems, it can be built natively.

### Linux (Native, Debian-based)
```
sudo apt install build-essential liblua5.1-dev
make
```

## Installation
Installation is also handled via [make](https://pubs.opengroup.org/onlinepubs/009695399/utilities/make.html).
By default, the program will be installed to `/usr/local/bin` but this can be changed via the `PREFIX` var.

```
sudo make install
# or
sudo make PREFIX=/usr install
```

## References
[Antirez's Kilo Editor](https://github.com/antirez/kilo)  
[Termios Manual Page](https://man7.org/linux/man-pages/man3/termios.3.html)  
[VT100 Escape Sequences](http://ascii-table.com/ansi-escape-sequences-vt-100.php)  
