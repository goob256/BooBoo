@echo off
pushd .
setlocal
call nca.bat %*
call nba.bat %*
call nia.bat %*
endlocal
popd
