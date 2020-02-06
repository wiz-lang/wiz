pushd snes
..\..\..\..\bin\wiz.exe -I../../../../common/snes/ -I../ main.wiz --system=wdc65816 -o ../snes_main.bin
@if %ERRORLEVEL% NEQ 0 goto error
popd
pushd gb
..\..\..\..\bin\wiz.exe -I../../../../common/gb/ -I../ main.wiz -o ../sgb_icd2_6003_test.gb
@if %ERRORLEVEL% NEQ 0 goto error
popd
@echo * Success!
@goto done
:error
@echo * Failed.
popd
:done
pause