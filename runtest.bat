@echo off

setlocal ENABLEDELAYEDEXPANSION

set /a tests=0
set /a successes=0
set /a failures=0


echo *** Running tests...

for /f %%i in ('dir /b test\*.6502.wiz 2^>nul') do (
    echo|set /p ="-   %%i... "
    wiz test\%%i -6502 -o tmp.o > tmp.log 2>&1

    if !errorlevel! equ 0 (
        echo OK
        set /a successes+=1
    ) else (
        echo FAILED ^(expected status 0, but got error status^)
        echo.
        echo Test Output:
        echo.
        type tmp.log
        echo.
        
        set /a failures+=1
    )

    set /a tests+=1
)

for /f %%i in ('dir /b test\*.gb.wiz 2^>nul') do (
    echo|set /p ="-   %%i... "
    wiz test\%%i -gb -o tmp.o > tmp.log 2>&1

    if !errorlevel! equ 0 (
        echo OK
        set /a successes+=1
    ) else (
        echo FAILED ^(expected status 0, but got error status^)
        echo.
        echo Test Output:
        echo.
        type tmp.log
        echo.
        
        set /a failures+=1
    )

    set /a tests+=1
)

for /f %%i in ('dir /b test\*.error.wiz 2^>nul') do (
    echo|set /p ="-   %%i... "
    wiz test\%%i -6502 -o tmp.o > tmp.log 2>&1

    if !errorlevel! neq 0 (
        echo OK
        set /a successes+=1
    ) else (
        echo FAILED ^(expected error status, but got status 0^).
        echo.
        echo Test Output:
        echo.
        type tmp.log
        echo.

        set /a failures+=1
    )

    set /a tests+=1
)

echo.
if exist tmp.o del tmp.o
if exist tmp.log del tmp.log

if %failures% == 0 (
    echo *** [SUCCESS] %successes% / %tests% test^(s^) passed.
    echo.
    call :exit 0
) else (
    echo *** [FAILURE] %failures% / %tests% test^(s^) failed, %successes% / %tests% passed.
    echo.
    call :exit 1
)

:exit
exit /b %1