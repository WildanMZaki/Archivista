@echo off
setlocal enabledelayedexpansion

:: ============================================
:: Archivista - Build Release Script
:: ============================================

echo ============================================
echo  Archivista - Build Release
echo ============================================
echo.

:: Step 1: Clean previous release build
if exist build\release rmdir /s /q build\release

:: Step 2: Configure CMake
echo [1/2] Configuring CMake...
cmake -S . -B build/release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

:: Step 3: Build
echo [2/2] Building...
cmake --build build/release -j
if !errorlevel! neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo  Build SUCCESS!
echo  Output: build\release\Archivista.exe
echo ============================================
echo.

:: Step 4: Ask to run
set /p "RUN_APP=Jalankan Archivista.exe? (y/n): "
if /i "!RUN_APP!" == "y" .\build\release\Archivista.exe

:: Step 5: Check gh CLI
echo.
where gh >nul 2>nul
if errorlevel 1 (
    echo [INFO] GitHub CLI gh tidak ditemukan, skip release.
    echo        Install di: https://cli.github.com/
    pause
    exit /b 0
)

set /p "DO_RELEASE=Upload sebagai GitHub Release? (y/n): "
if /i not "!DO_RELEASE!" == "y" (
    echo Selesai tanpa release.
    pause
    exit /b 0
)

:: Step 6: Gather release info
echo.
echo --- Release Info ---
set /p "TAG=Tag version (contoh: v0.1.0): "
set /p "TITLE=Release title (contoh: v0.1.0 - Menu Bar): "
set /p "NOTES=Release notes (deskripsi singkat): "

:: Step 7: Create release
echo.
echo Membuat release !TAG!...
gh release create !TAG! build\release\Archivista.exe --title "!TITLE!" --notes "!NOTES!"
if errorlevel 1 (
    echo [ERROR] Gagal membuat release!
    pause
    exit /b 1
)

echo.
echo ============================================
echo  Release !TAG! berhasil dibuat!
echo  Cek: https://github.com/WildanMZaki/Archivista/releases
echo ============================================
pause