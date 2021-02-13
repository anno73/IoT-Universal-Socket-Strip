@echo off

:: Call freerouter to auto route EDA (KiCad) board file (dsn) to Specctra Session file (ses).
:: 
:: https://github.com/freerouting/freerouting
:: https://github.com/freerouting/freerouting#using-the-command-line-arguments

set file=controller
set maxPasses=-mp 100

set source=-de "%CD%\%file%.dsn"
::set target=-do "%CD%\%file%.ses"

"%LOCALAPPDATA%\Freerouting\Freerouting.exe" %source% %target% %maxPasses%

if errorlevel 1 (
	echo There was an error. ERRORLEVEL is %errorlevel%.
	echo Does the source file exist?
	pause
	goto:eof
)

:: pause