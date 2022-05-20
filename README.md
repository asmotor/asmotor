# ASMotor

ASMotor is a portable and generic assembler engine and development system written in ANSI C99 and licensed under the GNU Public License v3. The package consists of the assembler, the librarian and the linker. It can be used as either a cross or native development system.

ASMotor is the spiritual successor to RGBDS, which was a fairly popular development package for the Game Boy. ASMotor is written by the original RGBDS author.

The assembler syntax is based on the friendly, well-known Motorola style macro language. ASMotor aims to be a friendly assembler and toolchain, well-suited to projects that incorporate a large body of hand crafted assembly code.

Currently supported CPUs are the 680x0 family, 6502, MIPS32, Z80, Game Boy, DCPU-16, CHIP-8/SCHIP and [the RC811 CPU core](https://github.com/QuorumComp/rc800).

In addition to raw binary dumps, the linker can output target specific binaries for a number of devices using these processors - various Commodore machines, Nintendo Game Boy, Sega Master System and Genesis and more.

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

Speaking of strings, **code and data literals** can be used to reduce clutter and improve readability. To load the register `a0` with the address of a string, you might do

```
lea { DC.B "This is a string",0 },a0
```

or to produce the address of a chunk of code

```
jsr {
	moveq #0,d0
	rts
}
```

**Complete macro language**. The macro language is based on Motorola's syntax and includes common extensions such as macros, repeating blocks of code and conditional assembly. While basic, the features have been tuned to allow for Turing completeness. Features for fixed point math are also present, it is for instance straightforward to generate a sine table at translation time using the macro language, or even the Mandelbrot set.


# Installing

## Prebuilt binary
Binaries can be found in the [releases section](https://github.com/asmotor/asmotor/releases).

There is no schedule for releases, and binaries are most likely somewhat older than the most recent commit. Releases happen after a period of dog fooding without any issues.

## Installing from source

The latest in-development version can easily be installed from source by following the directions below. Generally the master branch should be stable and safe to use.


### Windows
A script (```install.ps1```) is included that will install the compiled binaries into the `%USERPROFILE%\\bin` directory. This path should be added to your `%PATH%` for easier use. This script will also accept the
destination root (for instance ```%USERPROFILE%\\bin```). The installation directory should be added to your path variable.

To build from source, **cmake** must be installed. Installing cmake with [Chocolatey](https://chocolatey.org/) using the command

```
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
```

is suggested. A C compiler that cmake can find must also be installed. Visual Studio Community edition is suggested.

Then, using PowerShell, run the following commands:
```
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://raw.githubusercontent.com/asmotor/asmotor/master/install.ps1'))
```

### Linux and macOS
The project uses the [just](https://github.com/casey/just) command runner for project operations. An `install` recipe is available that will compile and install the compiled binaries into the `$HOME/.local/bin` directory. This path should be added to your `$PATH` for easier use. This recipe will also accept the destination root as an argument (for instance `/usr/local`).

For even easier installation on a supported platform, the latest version of ASMotor can be installed using the `install.sh` script. Currently supported platforms are ones with either `port` (MacPorts macOS), `brew` (Homebrew macOS/Linux), `apt-get` (Debian, Ubuntu and derivates), `dnf` (Fedora and friends) or `pacman` (Arch and derivates) installed. If the platform is not currently supported by `install.sh` it will complain and prompt you to install the necessary prerequisites `just`, `git` and `cmake` yourself.

```
curl https://raw.githubusercontent.com/asmotor/asmotor/master/install.sh | bash
```

If you want to install it globally, you can supply the installation prefix as a parameter:

```
curl https://raw.githubusercontent.com/asmotor/asmotor/master/install.sh | bash -s /usr/local "sudo"
```

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
