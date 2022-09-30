@echo off
pushd .
setlocal

set CFG="relwithdebinfo"
set TARGET="x"

:beginloop
if "%1"=="debug" goto debug
if "%1"=="release" goto release
if "%1"=="demo" goto endloop rem not supported in this script
if "%1"=="steam" goto endloop rem not supported in this script
if "%1"=="t" goto tgui6_flag
if "%1"=="s" goto shim4_flag
if "%1"=="g" goto game_flag
if "%1"=="d" goto data_flag
goto doneloop
:tgui6_flag
set TARGET="t"
goto endloop
:shim4_flag
set TARGET="s"
goto endloop
:game_flag
set TARGET="g"
goto endloop
:data_flag
set TARGET="d"
goto endloop
:release
set CFG="release"
goto endloop
:debug
set CFG="relwithdebinfo"
goto endloop
:endloop
shift
goto beginloop
:doneloop

if %TARGET%=="t" goto tgui6
if %TARGET%=="s" goto shim4
if %TARGET%=="g" goto game
if %TARGET%=="d" goto data

echo Invalid target: %TARGET%
goto done

:tgui6
cd c:\users\trent\code\tgui6\build
if %CFG%=="release" goto tgui6_release
rem copy relwithdebinfo\tgui6.dll ..\..\b
goto done
:tgui6_release
rem copy release\tgui6.dll ..\..\b
goto done
:shim4
cd c:\users\trent\code\shim4\build
if %CFG%=="release" goto shim4_release
rem copy relwithdebinfo\shim4.dll ..\..\b
goto done
:shim4_release
rem copy release\shim4.dll ..\..\b
goto done
:game
cd c:\users\trent\code\booboo\build
if %CFG%=="release" goto game_release
copy "relwithdebinfo\BooBoo.exe" ..\..\b
goto done
:game_release
copy "release\BooBoo.exe" ..\..\b
goto done
:data
if %CFG%=="release" goto data_release
cd c:\users\trent\code\b
xcopy /q /e /y ..\booboo\data data\
copy ..\booboo\docs\3rd_party.html .
goto done
:data_release
cd c:\users\trent\code\booboo\data
c:\users\trent\code\compress_dir\compress_dir.exe > nul
move ..\data.cpa c:\users\trent\code\b
copy ..\docs\3rd_party.html c:\users\trent\code\b
goto done
:done
endlocal
popd
