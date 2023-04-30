DeepRunner
==========

A 3D 6DoF underground maze shooter for the Silicon Graphics O2 workstation, and
other computer systems old and new.

Supported platforms:
  - SGI IRIX
  - GNU/Linux
  - FreeBSD
  - MS Windows

Status: project just started.

Website: http://nuclear.mutantstargoat.com/sw/games/deeprunner
Game page on itch.io: https://nuclear.itch.io/deeprunner
Source code repo: https://github.com/MutantStargoat/deeprunner

Controls
--------

### 6DoF Spaceball/Spacemouse
The game support 6DoF spacemouse input on SGI IRIX and other UNIX systems (like
GNU/Linux, FreeBSD, etc). Make sure the spacemouse driver is running and has
detected your 6dof device, before starting the game. You can use either the free
spacenav driver (https://spacenav.sf.net) or the official 3Dconnexion driver.

Use the spacemouse to navigate, and the regular mouse to shoot at enemies.

### Keyboard and Mouse
If you don't have a spacemouse, you can still run the game with just keyboard
and mouse. If you're not in full-screen mode, press `~` to capture the mouse and
start navigating; press it again to release the mouse.

Keybindings:

  - W/A/S/D: move forward/back and strafe left/right.
  - Q and E: roll left/right.
  - +/-: adjust sound volume.


License
-------
Copyright (C) 2023 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software. Feel free to use, modify and/or redistribute it
under the terms of the GNU General Public License v3, or at your option any
later version published by the Free Software Foundation. See COPYING for
details.

Build instructions
------------------
Data files are in a separate subversion repo. Grab them with:

    svn co svn://mutantstargoat.com/datadirs/deeprunner data

### UNIX build

Just type `make`.

On IRIX you can use any compiler and make utility, system make and MIPSPro are
supported as well as GCC and GNU make.

### Windows build

Install msys2, start a mingw32 shell, and type `make`. The windows version can
also be cross-compiled with `make crosswin`.

