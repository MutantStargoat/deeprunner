DeepRunner
==========

![Deep Runner logo](http://nuclear.mutantstargoat.com/sw/games/deeprunner/img/banner-sm.jpg)

A 3D 6DoF underground maze shooter for the Silicon Graphics O2 workstation, and
other computer systems old and new.

Supported platforms:
  - SGI IRIX
  - GNU/Linux
  - FreeBSD
  - MS Windows

 - Website: http://nuclear.mutantstargoat.com/sw/games/deeprunner
 - Game page on itch.io: https://nuclear.itch.io/deeprunner
 - Source code repo: https://github.com/MutantStargoat/deeprunner

Credits
-------
 - Code: John Tsiombikas (Nuclear).
 - Graphics/Levels: dstar.
 - Music: Robin Agani.

Controls
--------

### 6DoF Spaceball/Spacemouse
The game supports 6DoF spacemouse input on SGI IRIX and other UNIX systems
(GNU/Linux, FreeBSD, etc). Make sure the spacemouse driver is running and has
detected your 6dof device, before starting the game. You can use either the free
spacenav driver (https://spacenav.sf.net) or the official 3Dconnexion driver.

Use the spacemouse to navigate, and the regular mouse to shoot at enemies.

### Keyboard and Mouse
If you don't have a spacemouse, you can still run the game with just keyboard
and mouse. If you're not in full-screen mode, press `~` to capture the mouse and
start navigating; press it again to release the mouse.

Keybindings:

  - W/A/S/D: move forward/back and strafe left/right.
  - 2/X: move up/down.
  - Q and E: roll left/right.
  - ~: Capture/release the mouse (toggle mouselook).
  - Left mouse button: shoot lasers.
  - Right mouse button: shoot missiles.
  - +/-: adjust sound volume.
  - Alt-Enter: toggle between fullscreen/windowed.
  - Esc: quit game and return to the menu.

In future versions of the game, these will be re-bindable from an in-game menu.

Options
-------
Configuration options are read from `game.cfg`. If it doesn't exist, run the
game once and exit, and it will create one with the default options commented
out. Comments begin with the `#` symbol, to change one of the default options,
remove the `#` from the line and change the value. Here's a list of all options
and their possible values.

### video

Video output options:
 - `xres` and `yres`: set the resolution for windowed mode. Ignored in
   fullscreen.
 - `fullscreen`: 0 or 1 for windowed or fullscreen. Fullscreen does not switch
   video modes; resizes the window to cover the entire screen.
 - `vsync`: 0 or 1 for vsync off or on.

### gfx

Graphics options:
 - `drawdist`: set the draw distance in game units.
 - `texsize`: choose texture quality, value between 0 and 2.
 - `texfilter`: 0 = no filtering, 1 = bilinear, 2 = trilinear mipmapping.

### audio

Audio settings:
 - `volmaster`: master volume, 0 to 255.
 - `volmusic`: music volume, 0 to 255.
 - `volsfx`: sound effects volume, 0 to 255.
 - `music`: 0 to disable music, 1 to enable music.

### controls

Control options:
 - `invmousey`: set to 1 to invert the vertical mouse axis, 0 for normal.
 - `mousespeed`: mouse speed, 0 to 100.
 - `sballspeed`: 6dof spacemouse/spaceball speed, 0 to 100.


License
-------
### Code
Copyright (C) 2023 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software. Feel free to use, modify and/or redistribute it
under the terms of the GNU General Public License v3, or at your option any
later version published by the Free Software Foundation. See COPYING for
details.

### Graphics & levels
Copyright (C) 2023 dstar <dstar64@proton.me>

Game assets are released under the Creative Commons Attribution Share-Alike
license (CC BY-SA). See LICENSE.assets for details.

### Music
Copyright (C) 2023 Robin Agani <hello@robinagani.com>

Game music is released under the Creative Commons Attribution Share-Alike
license (CC BY-SA). See LICENSE.assets for details.


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

