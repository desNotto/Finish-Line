@echo off
rem Builds "Finish Line.vst3" (Release, x64). Requires Visual Studio 2022 (C++ workload) with CMake.
setlocal
set "CMAKE=cmake"
where cmake >nul 2>nul || set "CMAKE=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
"%CMAKE%" -S "%~dp0." -B "%~dp0build" -G "Visual Studio 17 2022" -A x64 || exit /b 1
"%CMAKE%" --build "%~dp0build" --config Release --target FinishLine_VST3 -- -m || exit /b 1
echo.
echo Built: %~dp0build\FinishLine_artefacts\Release\VST3\Finish Line.vst3
echo Copy that folder to "C:\Program Files\Common Files\VST3" to install.
