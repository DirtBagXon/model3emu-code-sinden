# Supermodel: Sega Model 3 Emulator (Sinden)
*Copyright 2003-2025 The Supermodel Team*

This is a fork of [model3emu-code](https://www.supermodel3.com) to add native **Sinden** light gun support.

This fork uses [ManyMouse](https://github.com/icculus/manymouse) to provide 2 player gun support in Linux and MacOSX.

Two player mouse input in Windows is currently unchanged _(supported in the official release)_ via  
the Windows Raw Input system.

Better support for MAME style multi ROM sets has also been added.

This repo will attempt to track the official git repo commits.

### Additional arguments:

    -game=<name>      Specific game to start in multi-romset
    -sinden=<n>       Sinden border configuration for gun games:
                      0=none [Default], 1=standard, 2=wide
    -nomousecursor    Disable desktop mouse cursor in SDL Windowed mode
    -abs-mice-only    Linux [evdev] absolute mouse movement filtering [lightguns]

**ManyMouse Notes:**

Compile _Linux/MacOSX_ with `INPUT_CFLAGS = -DSUPERMODEL_MANYMOUSE` in _Makefile_

_Linux/MacOSX_: Update _Supermodel.ini_ singular `MOUSE_` definitions to `MOUSEx_` _index_ definitions.

_MacOSX_ launcher, possibly `Terminal`, will require _"Input Monitoring"_ permissions.

---

For `arm` based systems, clone the `arm` branch:

    git clone --single-branch --branch arm https://github.com/DirtBagXon/model3emu-code-sinden.git

---
[![Two Player](https://img.youtube.com/vi/qplfGymaDPI/maxresdefault.jpg)](https://www.youtube.com/watch?v=qplfGymaDPI)

<!-- ![model3](screenshot.png) -->

## Overview

[Supermodel](https://supermodel3.com) emulates Sega's Model 3 arcade platform, allowing you to relive state-of-the-art 3D arcade gaming as it existed from 1996 through 1999. It uses OpenGL, [SDL2](https://libsdl.org), and can run on Windows, Linux, and macOS. It also supports network play on low-latency network connections. The source code is freely available under the terms of the [GNU General Public License](http://www.gnu.org/copyleft/gpl.html).

<p align="center">
  <img src="Docs/Images/Daytona2_1.gif" width="375" height="250" /> <img src="Docs/Images/LAMachin.gif" width="375" height="250" />
  <img src="Docs/Images/StarWars.gif" width="375" height="250" /> <img src="Docs/Images/FVipers2.gif" width="375" height="250" />
</p>

<img src="Docs/Images/Real3D_Logo.png" align="right" height="100"> Model 3 first made its debut in 1996 with Virtua Fighter 3 and Scud Race, and for the subsequent three years boasted the most powerful 3D hardware of any gaming platform. Developed by Real3D, then a Lockheed Martin company, and with a heritage rooted in advanced flight simulator technology, Model 3 featured capabilities that would not appear on PCs for several years. Using an on-board scene graph and geometry processor, it could store, transform, light, and rasterize tens of thousands of polygons per frame at a fluid 57.524 frames per second.

The aim of the Supermodel project is to develop an emulator that is both accurate and playable. As with virtually all arcade hardware, no public documentation for the Model 3 platform exists. What is known so far has been painstakingly reverse engineered from scratch.

## How To Get It

Builds from this repository are updated automatically and available on the Github [release page](https://github.com/DirtBagXon/model3emu-code-sinden/releases).

## Build Instructions

### Windows

The preferred method for building Supermodel is to use GCC and MSYS2. After installing [MSYS2](https://msys2.org), open the MSYS2 shell and install the required dependencies using the pacman package manager:

- GCC (```mingw64/mingw-w64-x86_64-gcc```)
- Make (```mingw64/mingw-w64-x86_64-make```)
- SDL2 (```mingw64/mingw-w64-x86_64-SDL2```, ```mingw64/mingw-w64-x86_64-SDL2_net```)

This can be done using the following commands:

```
pacman -S mingw64/mingw-w64-x86_64-gcc
pacman -S mingw64/mingw-w64-x86_64-make
pacman -S mingw64/mingw-w64-x86_64-SDL2
pacman -S mingw64/mingw-w64-x86_64-SDL2_net
```

At this point, you can continue using either the MSYS2 shell or Windows Command Prompt but ensure that both ```gcc``` and ```mingw32-make``` are in your path. In MSYS2, the location of these binaries will be ```/mingw64/bin``` and for Command Prompt, assuming MSYS2 was installed in the default location, add ```C:\msys64\mingw64\bin``` to your Windows ```PATH``` variable.

To build Supermodel without network support, use:

```
mingw32-make -f Makefiles/Makefile.Win32
```

For network support:

```
mingw32-make -f Makefiles/Makefile.Win32 NET_BOARD=1
```

### Linux

Ensure SDL2 is installed. Most package managers ought to have this available. For example, on Ubuntu, it should be sufficient to run:

```
sudo apt install libsdl2-dev
sudo apt install libsdl2-net-dev
```

And then build Supermodel:

```
make -f Makefiles/Makefile.UNIX
```

For network support:

```
make -f Makefiles/Makefile.UNIX NET_BOARD=1
```

### macOS

Ensure Apple's Xcode Command Line Tools are installed:

From a terminal:
```
xcode-select --install
```

Ensure SDL2 is installed.  Download the latest *.dmg files from both of the links below, and install per the READMEs in the .dmgs (i.e. in "/Library/Frameworks")

* SDL2: https://github.com/libsdl-org/SDL/releases

* SDL_net: https://github.com/libsdl-org/SDL_net/releases


And then build Supermodel:

```
make -f Makefiles/Makefile.OSX
```

For network support:

```
make -f Makefiles/Makefile.OSX NET_BOARD=1
```

### Note: running on macOS
If you try and run a macOS binary that was downloaded from the internet and/or built on a different machine, you need to grant macOS permission to execute the binary (just 1-time):

* Open the folder containing the binary in Finder, and right (or ctrl) click on it:

* Click "Open" when the following dialogue box appears : "macOS cannot verify the developer of “supermodel-git-xxxx”. Are you sure you want to open it?"

* Close the terminal window that opens (after clicking open)

Details: https://support.apple.com/guide/mac-help/open-a-mac-app-from-an-unidentified-developer-mh40616/mac
