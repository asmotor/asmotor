# ASMotor

ASMotor is a portable and generic assembler engine and development system written in ANSI C99 and licensed under the GNU Public License v3. The package consists of the assembler, the librarian and the linker. It can be used as either a cross or native development system.

The assembler syntax is based on the friendly, well-known Motorola style macro language. ASMotor aims to be a friendly assembler and toolchain, well-suited to projects that incorporate a large body of hand crafted assembly code.

Currently supported CPUs are the 680x0 family, 6502, MIPS32, Z80, Game Boy, DCPU-16, CHIP-8/SCHIP and [the RC811 CPU core](https://github.com/QuorumComp/rc800).

ASMotor is the spiritual successor to RGBDS, which was a fairly popular development package for the Game Boy. ASMotor is written by the original RGBDS author.

Interoperability is important and continues to improve. The linker will read ASMotor's own object format and ELF linkable objects, so it is easy to use a bit of C if necessary. The assembler can output ASMotor's own object format, Amiga hunk format (link and executable) and ELF. The assembler should slot right in to another toolchain without any trouble, if that is preferred.

# Why choose ASMotor?

**Mix different architectures** easily. No special tricks or other tools required, assemble a 6502 file and a Z80 file and have the linker output a ready to run Commodore 128 `.PRG`. ASMotor is also a great fit for Sega Genesis (Mega Drive) with its 68000 main CPU and Z80 subsystem.

**Cross-assemble or not**. The package is 100% portable ANSI C99 and will work as a native or cross assembler.

**Quality of life** built in:

**Flexible constant expressions**. All expressions, whose value the assembler's macro language does not need to know, are deferred to the linker. That means this contrived example is valid, because why shouldn't it be?

```
           moveq  #(MyConstant + SomebodyElsesExportedConstant) & $8F,d0

MyConstant EQU 4
           IMPORT SomebodyElsesExportedConstant
```

**String interpolation** is available with embedded expressions in string literals, for instance `"The result of 4*5 is {4*5}"` will produce the string `"The result of 4*5 is 20"`. Many formatting options and functions are available to use on both integers and strings.

**Code and data literals** are also useful, speaking of strings. To load the register `a0` with the address of a string, you might do

```
lea { DC.B "This is a string",0 },a0
```

or produce the address of some code

```
jsr {
	moveq #0,d0
	rts
}
```

**Complete macro language**. The macro language is based on Motorola's syntax and includes common extensions such as macros, repeating blocks of code and conditional assembly. While basic, the features have been tuned to allow for Turing completeness. Features for fixed point math are also present, it is for instance straightforward to generate a sine table using the macro language. Or how about the Mandelbrot set?


# Installing

## Building from source

### Windows
A script (```install.ps1```) is included that will install the compiled binaries into the %USERPROFILE%\\bin directory. This path should be added to your $PATH for easier use. This script will also accept the
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
