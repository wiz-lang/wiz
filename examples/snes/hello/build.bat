..\..\..\bin\wiz.exe -I../common/spc spc_main.wiz --system=spc700 -o spc_main.bin
@if ERRORLEVEL 1 goto error
..\..\..\bin\wiz.exe -I../common/snes main.wiz -o hello.sfc
@if ERRORLEVEL 1 goto error
@echo * Success!
@goto done
:error
@echo * Failed.
:done
pause