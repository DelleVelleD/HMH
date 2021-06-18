@echo off

pushd c:\github\hmh\src

@set SOURCES=win32_handmade.cpp
@set LIBS=user32.lib gdi32.lib
@set OUT_EXE=hmh64.exe

IF [%1]==[] GOTO DEBUG
IF [%1]==[-i] GOTO ONE_FILE
IF [%1]==[-l] GOTO LINK_ONLY
IF [%1]==[-r] GOTO RELEASE





REM ______________ DEBUG (compiles without optimization)

:DEBUG
ECHO [47m[94m%DATE% %TIME% Debug[0m[0m
@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /EHsc /nologo /Zi /MD /MP %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LIBS%
GOTO DONE

REM ______________ ONE FILE (compiles just one file with debug options, links with previosly created .obj files)

:ONE_FILE
ECHO [47m[94m%DATE% %TIME% One File (Debug)[0m[0m
IF [%~2]==[] ECHO "Place the .cpp path after using -i"; GOTO DONE;
ECHO [93mWarning: debugging might not work with one-file compilation[0m

@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /c /EHsc /nologo /Zi /MD %~2 /Fo%OUT_DIR%/
pushd ..\build\Debug
link /nologo *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM ______________ LINK ONLY (links with previosly created .obj files)

:LINK_ONLY
ECHO [47m[94m%DATE% %TIME% Link Only (Debug)[0m[0m
pushd ..\build
link /nologo *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM ______________ RELEASE (compiles with optimization)

:RELEASE
ECHO [47m[94m%DATE% %TIME% Release[0m[0m
@set OUT_DIR="..\build"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /EHsc /nologo /Zi /MD /O2 /MP %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LIBS%
GOTO DONE

:DONE
popd