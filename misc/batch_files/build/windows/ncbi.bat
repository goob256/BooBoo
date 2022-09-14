@echo off
pushd .
setlocal
call nc.bat %*
call nb.bat %*
call ni.bat %*
endlocal
popd
