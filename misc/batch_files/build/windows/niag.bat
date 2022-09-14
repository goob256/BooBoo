@echo off
pushd .
setlocal
call ni.bat t %1 %2
call ni.bat s %1 %2
call ni.bat g %1 %2
endlocal
popd
