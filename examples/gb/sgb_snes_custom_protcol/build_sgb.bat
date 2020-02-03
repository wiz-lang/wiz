pushd spc
..\..\..\..\bin\wiz.exe -I../../../../common/spc/ -I../ main.wiz --system=spc700 -o ../spc_main.bin
@if %ERRORLEVEL% NEQ 0 goto error
popd
pushd snes
..\..\..\..\bin\wiz.exe -I../../../../common/snes/ -Isgb/ -I../ main.wiz --system=wdc65816 -o ../snes_main.bin
@if %ERRORLEVEL% NEQ 0 goto error
popd
pushd gb
..\..\..\..\bin\wiz.exe -I../../../../common/gb/ -I../ main.wiz -o ../sgb_snes_custom_protocol.gb
@if %ERRORLEVEL% NEQ 0 goto error
popd
@echo * Success!
@goto done
:error
@echo * Failed.
popd
:done
pause