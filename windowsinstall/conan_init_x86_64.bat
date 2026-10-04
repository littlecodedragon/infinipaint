@echo off
cd %~dp0
cd ..
if not exist "%USERPROFILE%\.conan2\profiles\default" (
  conan profile detect --force
)
call .\conan\export_libs.bat
conan install . -of=build-x86_64 --build=missing -pr:h=conan/profiles/win-x86_64 -pr:b=default