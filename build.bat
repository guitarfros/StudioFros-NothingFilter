@echo off
rem Windows (Visual Studio 2022 or Build Tools + CMake).  Run from a Developer Command Prompt.
rem Usage: build.bat [-DJUCE_PATH=C:\path\to\JUCE]
cd /d "%~dp0"
cmake -B build %*
if errorlevel 1 exit /b 1
cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1
echo.
echo Built plug-in:
dir /b /ad build\NothingFilter_artefacts\Release\VST3
echo Copy the .vst3 folder to "C:\Program Files\Common Files\VST3" (needs administrator rights).
