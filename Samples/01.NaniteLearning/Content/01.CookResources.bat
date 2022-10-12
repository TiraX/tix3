@echo off & setlocal enabledelayedexpansion

rem Cook to Cooked/Windows directory

set CurrPath=%CD%
set BinaryPath=%CurrPath%\..\..\..\..\tix3\Binaries\Windows
set path=%path%;%BinaryPath%
set Converter=%BinaryPath%\TiXCooker.exe

if not exist "Cooked" (
	mkdir Cooked
	pushd "Cooked"
	if not exist "Windows" (
		mkdir "Windows"
	)
	popd
)

echo Converting tjs files.
for /r . %%i in (*.tjs) do (
  set B=%%i
  set Source=!B:%CD%\=!
  set Target=!Source:~0,-4!
  echo converting - !Source!
  %Converter% !Source! Cooked\Windows\!Target!.tasset -Force32BitIndex -ForceAlphaChannel
)

echo copy Config
pushd "Cooked\Windows"
if not exist "Config" (
	mkdir "Config"
)
popd
copy Config\*.ini Cooked\Windows\Config\

pause