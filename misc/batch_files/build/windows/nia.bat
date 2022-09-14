@echo off
pushd .
setlocal
call ni.bat t %*
call ni.bat s %*
call ni.bat g %*
call ni.bat d %*
endlocal
popd
