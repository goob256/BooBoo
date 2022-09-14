@echo off
pushd .
setlocal
call nba.bat %*
call nia.bat %*
endlocal
popd
