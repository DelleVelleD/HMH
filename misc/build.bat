@echo off
REM  HANDMADE_WIN64:    0 = not 64-bit windows        1 = build for 64-bit windows
REM  HANDMADE_SLOW:     0 = no slow code allowed!     1 = slow code allowed
REM  HANDMADE_INTERNAL: 0 = build for public release  1 = build for developer only

@set DEFINES=-DHANDMADE_WIN64=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1
@set WARNINGS=/W4 /WX /wd4201 /wd4100 /wd4189 /wd4706
@set CL_OPTIONS=/EHa- /nologo /MT /MP /Oi /GR- /Gm- /Fm %WARNINGS% %DEFINES% 
@set SOURCES=..\src\win32_handmade.cpp
@set DLL_SOURCES=..\src\handmade.cpp
@set LINK_OPTIONS=/nologo /opt:ref /incremental:no
@set DLL_LINK_OPTIONS=/opt:ref /incremental:no /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples /PDB:handmade_%random%.pdb 
@set LIBS=user32.lib gdi32.lib winmm.lib

REM _____________________________________________________________________________________________________
REM                                               START
REM _____________________________________________________________________________________________________

ctime -begin misc\hmh.ctm
IF NOT EXIST build mkdir build
pushd build

IF [%1]==[] GOTO DEBUG
IF [%1]==[-r] GOTO RELEASE

REM _____________________________________________________________________________________________________
REM                              DEBUG (compiles without optimization)
REM _____________________________________________________________________________________________________

:DEBUG
ECHO %DATE% %TIME%    Debug
ECHO ---------------------------------

del *.pdb > NUL 2> NUL
ECHO Waiting for PDB > lock.tmp
cl /Z7 /Od %CL_OPTIONS% %DLL_SOURCES% /LD /link %DLL_LINK_OPTIONS%
del lock.tmp
cl /Z7 /Od %CL_OPTIONS% %SOURCES% /link %LINK_OPTIONS% %LIBS%
GOTO DONE

REM _____________________________________________________________________________________________________
REM                                 RELEASE (compiles with optimization)
REM _____________________________________________________________________________________________________

:RELEASE
ECHO %DATE% %TIME%    Release
ECHO ---------------------------------

cl /O2 %CL_OPTIONS% %DLL_SOURCES% /LD /link %DLL_LINK_OPTIONS%
cl /O2 %CL_OPTIONS% %SOURCES% /link %LINK_OPTIONS% %LIBS%
GOTO DONE

REM _____________________________________________________________________________________________________
REM                                               DONE
REM _____________________________________________________________________________________________________

:DONE
ECHO ---------------------------------
popd
ctime -end misc\hmh.ctm