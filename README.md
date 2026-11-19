## LuaJIT Decompiler v2 (Linux)

*LuaJIT Decompiler v2* is a replacement tool for the old and now mostly defunct python decompiler.  
The project fixes all of the bugs and quirks the python decompiler had while also offering  
full support for gotos and stripped bytecode including locals and upvalues.

This repository is a **Linux port** of the original [LuaJIT Decompiler v2](https://github.com/marsinator358/luajit-decompiler-v2) by [marsinator358](https://github.com/marsinator358), which was written for Windows. The decompilation logic is unchanged; only the Windows-specific platform code (Win32 file/console APIs, dialogs, and build) has been replaced with POSIX equivalents and a CMake build.

## Usage

1. Head to the release section and download the latest executable.
2. Run the program in a terminal. Use `-?` to show usage and options.
   Example: `./luajit-decompiler-v2 /path/to/input [options]`
3. All successfully decompiled `.lua` files are placed by default into the `output` folder
located in the same directory as the executable.

## Building (Linux)

```bash
cmake -B build
cmake --build build -j
# Binary lands at build/luajit-decompiler-v2
```

Feel free to [report any issues](https://github.com/fredericodeveloper/luajit-decompiler-v2-linux/issues/new) you have with this Linux port.

## TODO

* bytecode big endian support
* improved decompilation logic for conditional assignments

---

This project uses an boolean expression decompilation algorithm that is based on this paper:  
[www.cse.iitd.ac.in/~sak/reports/isec2016-paper.pdf](https://www.cse.iitd.ac.in/~sak/reports/isec2016-paper.pdf)
