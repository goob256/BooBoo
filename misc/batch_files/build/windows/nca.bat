@echo off
pushd .
setlocal
call nc.bat t %*
call nc.bat s %*
call nc.bat g %*
call nc.bat d %*
endlocal
popd
