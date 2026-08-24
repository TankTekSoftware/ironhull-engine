
@echo off
setlocal enabledelayedexpansion

rem Capture the script name before any "shift" below, since plain "shift"
rem shifts %0 too, which would otherwise corrupt %~n0 in the usage banner.
set "SCRIPT_NAME=%~n0"

rem Default preset (matches Makefile default)
set "PRESET=desktop-debug"

rem Parse arguments: action (build/configure/clean/rebuild) and KEY=VALUE pairs
rem
rem Note: cmd.exe treats "=" as an argument separator on the raw command line,
rem the same as a space, so an unquoted "PRESET=desktop-release" arrives here
rem as two separate args ("PRESET" then "desktop-release"), not one. We handle
rem both that split form and the quoted single-token form ("PRESET=desktop-release").
set "ACTION="
:parse_loop
if "%~1"=="" goto parsed
if "%~1"=="-h" goto usage
if "%~1"=="--help" goto usage

set "TOK=%~1"
echo "%TOK%" | findstr "=" >nul
if %ERRORLEVEL% EQU 0 (
	rem Single token already contains "=" (e.g. it was quoted).
	for /f "tokens=1* delims==" %%A in ("%TOK%") do (
		if /I "%%A"=="PRESET" (
			set "PRESET=%%B"
		) else (
			set "%%A=%%B"
		)
	)
	shift
) else (
	set "IS_ACTION="
	for %%W in (all build configure clean rebuild) do (
		if /I "%TOK%"=="%%W" set "IS_ACTION=1"
	)
	if defined IS_ACTION (
		if not defined ACTION set "ACTION=%TOK%"
		shift
	) else if not "%~2"=="" (
		rem TOK is the KEY half of a KEY=VALUE pair split apart by cmd.exe;
		rem %~2 is the VALUE half.
		if /I "%TOK%"=="PRESET" (
			set "PRESET=%~2"
		) else (
			set "%TOK%=%~2"
		)
		shift
		shift
	) else (
		if not defined ACTION set "ACTION=%TOK%"
		shift
	)
)
goto parse_loop

:parsed
if not defined ACTION set "ACTION=build"

if /I "%ACTION%"=="all" set "ACTION=build"

if /I "%ACTION%"=="configure" (
	cmake --preset=%PRESET%
	exit /b %ERRORLEVEL%
)

if /I "%ACTION%"=="build" (
	cmake --preset=%PRESET%
	if errorlevel 1 exit /b %ERRORLEVEL%
	cmake --build --preset=%PRESET%
	exit /b %ERRORLEVEL%
)

if /I "%ACTION%"=="clean" (
	cmake --build --preset=%PRESET% --target clean
	exit /b %ERRORLEVEL%
)

if /I "%ACTION%"=="rebuild" (
	cmake --build --preset=%PRESET% --target clean
	if errorlevel 1 exit /b %ERRORLEVEL%
	cmake --preset=%PRESET%
	if errorlevel 1 exit /b %ERRORLEVEL%
	cmake --build --preset=%PRESET%
	exit /b %ERRORLEVEL%
)

echo Unknown action "%ACTION%".
goto usage

:usage
echo Usage: %SCRIPT_NAME% [action] [KEY=VALUE]...
echo.
echo Actions:
echo   build       Configure and build (default)
echo   configure   Run cmake --preset=PRESET
echo   clean       Run cmake --build --preset=PRESET --target clean
echo   rebuild     Clean + configure + build
echo.
echo Examples:
echo   %SCRIPT_NAME% build PRESET=desktop-release
echo   %SCRIPT_NAME% build PRESET=desktop-debug-release
exit /b 1
