# Microsoft Developer Studio Project File - Name="deeprun_glide" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=deeprun_glide - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "deeprun_glide.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "deeprun_glide.mak" CFG="deeprun_glide - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "deeprun_glide - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "deeprun_glide - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "deeprun_glide - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W2 /GX /O2 /I "src" /I "src/glidew32" /I "libs" /I "libs/imago/src" /I "libs/treestor/include" /I "libs/goat3d/include" /I "libs/drawtext" /I "libs/mikmod/include" /I "libs/glide" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "MIKMOD_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib glide2x.lib winmm.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "deeprun_glide - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "deeprun_glide___Win32_Debug"
# PROP BASE Intermediate_Dir "deeprun_glide___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "deeprun_glide___Win32_Debug"
# PROP Intermediate_Dir "deeprun_glide___Win32_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W2 /Gm /GX /ZI /Od /I "src" /I "src/glidew32" /I "libs" /I "libs/imago/src" /I "libs/treestor/include" /I "libs/goat3d/include" /I "libs/drawtext" /I "libs/mikmod/include" /I "libs/glide" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "MIKMOD_STATIC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib glide2x.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "deeprun_glide - Win32 Release"
# Name "deeprun_glide - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "gaw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gaw\gaw.h
# End Source File
# Begin Source File

SOURCE=.\src\gaw\gaw_glide.c
# End Source File
# Begin Source File

SOURCE=.\src\gaw\gawswtnl.c
# End Source File
# Begin Source File

SOURCE=.\src\gaw\gawswtnl.h
# End Source File
# Begin Source File

SOURCE=.\src\gaw\polyclip.c
# End Source File
# Begin Source File

SOURCE=.\src\gaw\polyclip.h
# End Source File
# End Group
# Begin Group "glidew32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\glidew32\main_grw32.c
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

SOURCE=.\src\util.c
# End Source File
# Begin Source File

SOURCE=.\src\util.h
# End Source File
# End Group
# End Target
# End Project
