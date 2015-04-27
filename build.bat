@echo off


:: A poor alternative to directory globbing.
setlocal ENABLEDELAYEDEXPANSION
set files=
for %%j in (src\js\wiz src\js\wiz\ast src\js\wiz\cpu src\js\wiz\cpu\gameboy src\js\wiz\cpu\mos6502 src\js\wiz\compile src\js\wiz\fs src\js\wiz\parse src\js\wiz\sym) do (
    set directory=%%j%
    for /f %%i in ('dir /b !directory!\*.js') do set "files=!files!!directory!\%%i "
)
:: Minify the JS version of the compiler.
::type %files% 2> nul | jsmin > src\js\editor\dependencies\wiz.min.js
type %files% 2> nul > src\js\editor\dependencies\wiz.min.js

:: A poor alternative to directory globbing.
setlocal ENABLEDELAYEDEXPANSION
set files=
for %%j in (src\d\wiz src\d\wiz\ast src\d\wiz\cpu src\d\wiz\cpu\gameboy src\d\wiz\cpu\mos6502 src\d\wiz\sym src\d\wiz\parse src\d\wiz\compile) do (
    set directory=%%j%
    for /f %%i in ('dir /b !directory!\*.d') do set "files=!files!!directory!\%%i "
)

:: Build the compiler.
dmd %files% -Isrc\d -ofwiz -g -debug
if %errorlevel% neq 0 call :exit 1
:: Compiler test.
::call runtest.bat
::if %errorlevel% neq 0 call :exit 1
::echo.
:: Compile a test program.
wiz examples/gameboy/snake/snake.wiz -gb -sym -o examples/gameboy/snake/snake.gb
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program.
wiz examples/nes/hello/hello.wiz -6502 -o examples/nes/hello/hello.nes
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program.
wiz examples/nes/scroller/scroller.wiz -6502 -o examples/nes/scroller/scroller.nes
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program.
wiz examples/nes/parallax/main.wiz -6502 -nl -o examples/nes/parallax/parallax.nes
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program.
wiz examples/gameboy/selfpromo/main.wiz -gb -o examples/gameboy/selfpromo/selfpromo.gb
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program.
wiz examples/gameboy/shmup/main.wiz -gb -o examples/gameboy/shmup/shmup.gb
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program.
wiz examples/gameboy/supaboy/snes_main.wiz -6502 -o examples/gameboy/supaboy/snes_main.6502
if %errorlevel% neq 0 call :exit 1
echo.
:: Compile another test program, depends on the SNES driver
wiz examples/gameboy/supaboy/main.wiz -gb -o examples/gameboy/supaboy/supaboy.gb
if %errorlevel% neq 0 call :exit 1
echo.

:: Success!
if exist build2.bat (
    build2.bat
) else (
    call :exit 0
)
call :exit 0

:exit
:: Sleep so that sublime text will not write a [Finished] message in the middle of a compiler message.
ping 1.1.1.1 -n 1 -w 10 > nul
echo.
if %1% equ 0 (
    echo --- Build success. ---
) else (
    echo --- Build failure. ---
pause
)
exit %1