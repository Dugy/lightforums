# Lightforums

Lightforums is a forum software striving for minimal requirements, designed for smaller discussion sites. It is structured around a Reddit-style nested replying to post. Posts support a lightweight markup language.

**This is a minimal usable version.**

## Features
* Small requirements thanks to Wt, allowing it to run with on devices with weak hardware, like mobile phones
* No database usage to further reduce requirements
* Support for logging-in, users, secure savign of passwords and such
* Posts can be rated by users
* All strings shown can be changed or translated through its online form
* Easily separable parts, it is designed as a library and a class that uses it; the appearance can be altered by editing that class and even incorporated into another Wt-based project

## Limitations
* Many useful functions missing
* Not tested on Windows, some problems might need fixing (but in theory, it should be able to run)

## Setup
Currently, only a qt project file is supplied. QtCreator can create CMake files from it and compile. Creating a CMake file for this project is trivial anyway. It compiles into a standalone program that acts as a web server. If compiled differently, it can be used with regular web servers (see Wt documentation for more). The `resources` folder must be in the folder where the program is run.

The only dependency is Wt. It is available on Ubuntu as `witty` package.

## Licence
Open source, if you need a commercial one, contact me.
_Warning: The files in the `resources` folder are currently copied from the Wt examples, it will be changed later._
