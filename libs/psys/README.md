libpsys
=======

Overview
--------
Libpsys is an easy to use, reusable, and extensible particle system library.

main project site: http://github.com/jtsiomb/libpsys

The three major design ideas behind libpsys are:

 - Ability to easily animate any particle system parameter, treating all
   internally as keyframe tracks (thanks to libanim).
 - Complete separation of particle system simulation and rendering. Which makes
   libpsys trivially easy to use in conjunction with OpenGL, Direct3D, custom
   software rasterizers, or even raytracers.
 - Maintain a simple, orthogonal, easy to use, and intuitive API. 

Libpsys depends on:
 - libanim: http://github.com/jtsiomb/libanim
 - libimago: http://github.com/jtsiomb/libimago

Current libpsys has been modified to drop dependency to libvmath, and to make it
compatible with the new libanim 2.0, which did likewise. Libpsys version number
has been also bumped to 2.0 to signify this dependency relationship with libanim
2.0, and also because these changes make it binary and source incompatible with
earlier libpsys versions.


License
-------
Copyright (C) 2014-2018 John Tsiombikas <nuclear@member.fsf.org>

libpsys is free software, released under the terms of the GNU Lesser General
Public License v3 (or at your option, any later version published by the Free
Software Foundation). Read COPYING and COPYING.LESSER for details.
