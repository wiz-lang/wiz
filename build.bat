@echo off

:: A poor alternative to directory globbing.
setlocal ENABLEDELAYEDEXPANSION
set files=
for %%j in (src\wiz src\wiz\ast src\wiz\cpu src\wiz\cpu\gameboy src\wiz\sym src\wiz\parse src\wiz\compile) do (
    set directory=%%j%
    for /f %%i in ('dir /b !directory!\*.d') do set "files=!files!!directory!\%%i "
)

:: Build the compiler.
dmd %files% -Isrc -ofwiz -g -debug
if %errorlevel% neq 0 call :exit 1
:: Compile a test program.
wiz examples/gameboy/snake/demo.wiz
if %errorlevel% neq 0 call :exit 1
:: Success!
call :exit 0

:exit
:: Sleep so that sublime text will not write a [Finished] message in the middle of a compiler message.
ping 1.1.1.1 -n 1 -w 10 > nul
echo.
if %1% equ 0 (
    echo --- Build success. ---
) else (
    echo --- Build failure. ---
)
exit %1