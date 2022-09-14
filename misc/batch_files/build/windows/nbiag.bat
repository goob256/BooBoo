@echo off
pushd .
setlocal
call nba.bat %1
call niag.bat %1 %2
endlocal
popd
