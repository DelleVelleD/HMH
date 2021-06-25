@echo off
pushd src

REM  HANDMADE_WIN64:    0 = not 64-bit windows        1 = build for 64-bit windows
REM  HANDMADE_SLOW:     0 = no slow code allowed!     1 = slow code allowed
REM  HANDMADE_INTERNAL: 0 = build for public release  1 = build for developer only

@set DEFINES=-DHANDMADE_WIN64=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1
@set WARNINGS=/W4 /WX /wd4201 /wd4100 /wd4189 /wd4706
@set CL_OPTIONS=/EHa- /nologo /MT /MP /Oi /GR- /Gm- /Fm %WARNINGS%
@set LINK_OPTIONS=/nologo /opt:ref
@set SOURCES=win32_handmade.cpp
@set LIBS=user32.lib gdi32.lib winmm.lib
@set OUT_EXE=hmh64.exe

IF [%1]==[] GOTO DEBUG
IF [%1]==[-i] GOTO ONE_FILE
IF [%1]==[-l] GOTO LINK_ONLY
IF [%1]==[-r] GOTO RELEASE

REM _____________________________________________________________________________________________________
REM                              DEBUG (compiles without optimization)
REM _____________________________________________________________________________________________________

:DEBUG
ECHO %DATE% %TIME%    Debug
ECHO ---------------------------------

@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /Z7 /Od %CL_OPTIONS% %DEFINES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_OPTIONS% %LIBS%
GOTO DONE

REM _____________________________________________________________________________________________________
REM    ONE FILE (compiles just one file with debug options, links with previosly created .obj files)
REM _____________________________________________________________________________________________________

:ONE_FILE
ECHO %DATE% %TIME%    One File (Debug)
ECHO ---------------------------------
IF [%~2]==[] ECHO "Place the .cpp path after using -i"; GOTO DONE;
ECHO [93mWarning: debugging might not work with one-file compilation[0m

@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /c /Z7 %CL_OPTIONS% %DEFINES% %~2 /Fo%OUT_DIR%/
pushd ..\build
link %LINK_OPTIONS% *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM _____________________________________________________________________________________________________
REM                           LINK ONLY (links with previosly created .obj files)
REM _____________________________________________________________________________________________________

:LINK_ONLY
ECHO %DATE% %TIME%    Link Only (Debug)
ECHO ---------------------------------
pushd ..\build
link %LINK_OPTIONS% *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM _____________________________________________________________________________________________________
REM                                 RELEASE (compiles with optimization)
REM _____________________________________________________________________________________________________

:RELEASE
ECHO %DATE% %TIME%    Release
ECHO ---------------------------------

@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /O2 %CL_OPTIONS% %DEFINES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_OPTIONS% %LIBS%
GOTO DONE



:DONE
ECHO ---------------------------------
popd