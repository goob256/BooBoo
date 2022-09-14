@echo off
pushd .
setlocal
call nb.bat t %*
call nb.bat s %*
call nb.bat g %*
endlocal
popd
