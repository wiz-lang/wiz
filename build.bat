@echo off
setlocal ENABLEDELAYEDEXPANSION
set files=
for %%j in (src\wiz src\wiz\ast src\wiz\parse src\wiz\compile src\wiz\def) do (
    set directory=%%j%
    for /f %%i in ('dir /b !directory!\*.d') do set "files=!files!!directory!\%%i "
)
dmd %files% -Isrc -ofwiz
if %errorlevel% neq 0 goto err

wiz examples/test.wiz

goto done
:err
exit 1

:done
:: Sleep so that sublime text will not write a [Finished] message in the middle of a compile error message.
ping 1.1.1.1 -n 1 -w 200 > nul