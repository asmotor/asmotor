# ASMotor

ASMotor is a portable and generic assembler engine and development system written in ANSI C99 and licensed under the GNU Public License v3. The package consists of the assembler, the librarian and the linker. It can be used as either a cross or native development system.

The assembler syntax is based on the Motorola style macro language.

Currently supported CPUs are the 680x0 family, 6502, MIPS32, Z80, Game Boy, DCPU-16 and CHIP-8/SCHIP.

ASMotor is the spiritual successor to RGBDS, which was a fairly popular development package for the Game Boy. ASMotor is written by the original RGBDS author.

# Installing

## Building from source

### Windows
A script (```install.sh```) is included that will install the compiled binaries into the %USERPROFILE%\\bin directory. This path should be added to your $PATH for easier use. This script will also accept the
destination root (for instance ```%USERPROFILE\\bin```). The installation directory should be added to your path variable.

To build from source, cmake must be installed. Installing cmake with [Chocolatey](https://chocolatey.org/) using command ```choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'``` is suggested. A C compiler that cmake can find must also be installed. Visual Studio Community edition is suggested.

Then, using PowerShell, run the following commands:
```
	Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
	install.ps1
```

### Linux and macOS
A script (```install.sh```) is included that will install the compiled binaries into the $HOME/bin directory. This path should be added to your $PATH for easier use. This script will also accept the
destination root (for instance ```/usr/local```).

For even easier installation, provided you have the necessary prerequisites, ```git``` and ```cmake```, installed, the latest version of ASMotor can be installed using

```
    curl https://raw.githubusercontent.com/asmotor/asmotor/master/bootstrap.sh | sh
```

If you want to install it globally, you can supply the installation prefix as a parameter:

```
    curl https://raw.githubusercontent.com/asmotor/asmotor/master/bootstrap.sh | sh -s /usr/local
```

To install ```git``` and ```cmake```, it is suggested you use your distribution's package manager. For macOS, use [brew](https://brew.sh) or [MacPorts](https://www.macports.org).

# Editing code
A VSCode extension that enables syntax highlighting is also available at https://marketplace.visualstudio.com/items?itemName=ASMotor.asmotor-syntax

# Further reading
Dive into the documentation to learn more about:

* [Introduction](doc/Introduction.md), goals and background
* [Invoking the assembler](doc/Assembler.md) and basic syntax
* [Symbols](doc/Symbols.md) and labels
* [Control structures](doc/ControlStructures.md) like ```INCLUDE```, ```MACRO```'s and conditional assembling.
* [Expressions](doc/Expressions.md) and how they're built
* [Printing diagnostic messages](doc/Diagnostics.md), warnings and errors
* [Organising code](doc/OrganisingCode.md) into sections. How to define data.
* [The linker](doc/Linker.md)

## Index and reference
* [CPU specific](doc/CpuSpecifics.md) details
* [Index of all directives](doc/IndexDirectives.md)
* [Index of all functions](doc/IndexFunctions.md)
* [Operator reference](doc/ReferenceOperators.md)
* [String member reference](doc/ReferenceStringMembers.md)
