# Microsoft Developer Studio Project File - Name="deeprun" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=deeprun - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "deeprun.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "deeprun.mak" CFG="deeprun - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "deeprun - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "deeprun - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "deeprun - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W2 /GX /O2 /I "src" /I "src/opengl" /I "libs" /I "libs/imago/src" /I "libs/treestor/include" /I "libs/goat3d/include" /I "libs/drawtext" /I "libs/mikmod/include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "MINIGLUT_USE_LIBC" /D "MIKMOD_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib winmm.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "deeprun - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W2 /Gm /GX /ZI /Od /I "src" /I "src/opengl" /I "libs" /I "libs/imago/src" /I "libs/treestor/include" /I "libs/goat3d/include" /I "libs/drawtext" /I "libs/mikmod/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "MINIGLUT_USE_LIBC" /D "MIKMOD_STATIC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "deeprun - Win32 Release"
# Name "deeprun - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "gaw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gaw\gaw.h
# End Source File
# Begin Source File

SOURCE=.\src\gaw\gaw_gl.c
# End Source File
# End Group
# Begin Group "opengl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opengl\main_gl.c
# End Source File
# Begin Source File

SOURCE=.\src\opengl\miniglut.c
# End Source File
# Begin Source File

SOURCE=.\src\opengl\miniglut.h
# End Source File
# Begin Source File

SOURCE=.\src\opengl\opengl.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\audio.c
# End Source File
# Begin Source File

SOURCE=.\src\audio.h
# End Source File
# Begin Source File

SOURCE=.\src\config.h
# End Source File
# Begin Source File

SOURCE=.\src\darray.c
# End Source File
# Begin Source File

SOURCE=.\src\darray.h
# End Source File
# Begin Source File

SOURCE=.\src\enemy.c
# End Source File
# Begin Source File

SOURCE=.\src\enemy.h
# End Source File
# Begin Source File

SOURCE=.\src\game.c
# End Source File
# Begin Source File

SOURCE=.\src\game.h
# End Source File
# Begin Source File

SOURCE=.\src\geom.c
# End Source File
# Begin Source File

SOURCE=.\src\geom.h
# End Source File
# Begin Source File

SOURCE=.\src\gfxutil.c
# End Source File
# Begin Source File

SOURCE=.\src\gfxutil.h
# End Source File
# Begin Source File

SOURCE=.\src\input.c
# End Source File
# Begin Source File

SOURCE=.\src\input.h
# End Source File
# Begin Source File

SOURCE=.\src\level.c
# End Source File
# Begin Source File

SOURCE=.\src\level.h
# End Source File
# Begin Source File

SOURCE=.\src\loading.c
# End Source File
# Begin Source File

SOURCE=.\src\loading.h
# End Source File
# Begin Source File

SOURCE=.\src\mesh.c
# End Source File
# Begin Source File

SOURCE=.\src\mesh.h
# End Source File
# Begin Source File

SOURCE=.\src\meshgen.c
# End Source File
# Begin Source File

SOURCE=.\src\mtltex.c
# End Source File
# Begin Source File

SOURCE=.\src\mtltex.h
# End Source File
# Begin Source File

SOURCE=.\src\octree.c
# End Source File
# Begin Source File

SOURCE=.\src\octree.h
# End Source File
# Begin Source File

SOURCE=.\src\options.c
# End Source File
# Begin Source File

SOURCE=.\src\font.c
# End Source File
# Begin Source File

SOURCE=.\src\options.h
# End Source File
# Begin Source File

SOURCE=.\src\player.c
# End Source File
# Begin Source File

SOURCE=.\src\player.h
# End Source File
# Begin Source File

SOURCE=.\src\rbtree.c
# End Source File
# Begin Source File

SOURCE=.\src\rbtree.h
# End Source File
# Begin Source File

SOURCE=.\src\rendlvl.c
# End Source File
# Begin Source File

SOURCE=.\src\rendlvl.h
# End Source File
# Begin Source File

SOURCE=.\src\scr_debug.c
# End Source File
# Begin Source File

SOURCE=.\src\scr_game.c
# End Source File
# Begin Source File

SOURCE=.\src\scr_logo.c
# End Source File
# Begin Source File

SOURCE=.\src\scr_menu.c
# End Source File
# Begin Source File

SOURCE=.\src\scr_opt.c
# End Source File
# Begin Source File

SOURCE=.\src\util.c
# End Source File
# Begin Source File

SOURCE=.\src\util.h
# End Source File
# End Group
# Begin Group "cgmath"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libs\cgmath\cgmath.h
# End Source File
# Begin Source File

SOURCE=.\libs\cgmath\cgmmat.inl
# End Source File
# Begin Source File

SOURCE=.\libs\cgmath\cgmmisc.inl
# End Source File
# Begin Source File

SOURCE=.\libs\cgmath\cgmquat.inl
# End Source File
# Begin Source File

SOURCE=.\libs\cgmath\cgmray.inl
# End Source File
# Begin Source File

SOURCE=.\libs\cgmath\cgmvec3.inl
# End Source File
# Begin Source File

SOURCE=.\libs\cgmath\cgmvec4.inl
# End Source File
# End Group
# End Target
# End Project
