@echo off
pushd .
setlocal
if "%1"=="t" goto tgui6
if "%1"=="s" goto shim4
if "%1"=="g" goto game
:tgui6
call nb.bat t %2
call ni.bat t %2 %3 %4
goto done
:shim4
call nb.bat s %2
call ni.bat s %2 %3 %4
goto done
:game
call nb.bat g %2
call ni.bat g %2 %3 %4
goto done
:done
endlocal
popd
