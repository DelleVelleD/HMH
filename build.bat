@echo off
pushd src

REM  HANDMADE_WIN64:    0 = not 64-bit windows        1 = build for 64-bit windows
REM  HANDMADE_SLOW:     0 = no slow code allowed!     1 = slow code allowed
REM  HANDMADE_INTERNAL: 0 = build for public release  1 = build for developer only

@set DEFINES=-DHANDMADE_WIN64=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1
@set WARNINGS=/W4 /WX /wd4201 /wd4100 /wd4189 /wd4706
@set CL_OPTIONS=/EHa- /nologo /MT /MP /Oi /GR- /Gm- /Fm %WARNINGS% %DEFINES% 
@set LINK_OPTIONS=/nologo /opt:ref /incremental:no
@set DLL_SOURCES=handmade.cpp
@set SOURCES=win32_handmade.cpp
@set LIBS=user32.lib gdi32.lib winmm.lib
@set OUT_DLL=handmade.dll
@set OUT_EXE=win32_handmade.exe

IF [%1]==[] GOTO DEBUG
IF [%1]==[-r] GOTO RELEASE

REM _____________________________________________________________________________________________________
REM                              DEBUG (compiles without optimization)
REM _____________________________________________________________________________________________________

:DEBUG
ECHO %DATE% %TIME%    Debug
ECHO ---------------------------------

@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /Z7 /Od %CL_OPTIONS% %DLL_SOURCES% /Fe%OUT_DIR%/%OUT_DLL% /Fo%OUT_DIR%/ /LD /link /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples
cl /Z7 /Od %CL_OPTIONS% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_OPTIONS% %LIBS%
GOTO DONE

REM _____________________________________________________________________________________________________
REM                                 RELEASE (compiles with optimization)
REM _____________________________________________________________________________________________________

:RELEASE
ECHO %DATE% %TIME%    Release
ECHO ---------------------------------

@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /O2 %CL_OPTIONS% %DLL_SOURCES% /Fo%OUT_DIR%/ /DLL /link /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples
cl /O2 %CL_OPTIONS% %SOURCES% /Fo%OUT_DIR%/ /link %LINK_OPTIONS% %LIBS%
GOTO DONE



:DONE
ECHO ---------------------------------
popd