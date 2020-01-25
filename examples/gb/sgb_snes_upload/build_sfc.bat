pushd spc
..\..\..\..\bin\wiz.exe -I../../../../common/spc/ -I../ main.wiz --system=spc700 -o ../spc_main.bin
@if ERRORLEVEL 1 goto error
popd
pushd snes
..\..\..\..\bin\wiz.exe -I../../../../common/snes/ -Isfc/ -I../ main.wiz -o ../snes_main.sfc
@if ERRORLEVEL 1 goto error
popd
@echo * Success!
@goto done
:error
@echo * Failed.
popd
:done
pause