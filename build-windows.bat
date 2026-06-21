@echo off
setlocal EnableDelayedExpansion
chcp 65001 > nul

echo ================================================
echo  EncFS MP CLI - Windows Build Script
echo ================================================
echo.

REM Check for MSYS2/MinGW
where gcc >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] MinGW not found!
    echo.
    echo Please install MSYS2 from: https://www.msys2.org/
    echo Then run: pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-openssl
    echo.
    echo After installation, open "MSYS2 MinGW 64-bit" terminal and run this script again.
    pause
    exit /b 1
)

echo [INFO] MinGW found
echo.

REM Set MSYS2 path if not set
if not defined MSYSTEM (
    set "PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%"
)

REM Go to script directory
cd /d "%~dp0"
set ROOT_DIR=%CD%

REM Create build directory
echo [STEP 1/5] Creating build directory...
if not exist build mkdir build
cd build
echo.

REM Configure with CMake
echo [STEP 2/5] Configuring CMake...
cmake .. ^
    -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_COMPILER=gcc ^
    -DCMAKE_CXX_COMPILER=g++ ^
    -DCMAKE_RC_COMPILER=windres ^
    -DCMAKE_INSTALL_PREFIX=install ^
    -DBUILD_SHARED_LIBS=OFF

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)
echo.

REM Build
echo [STEP 3/5] Building...
mingw32-make -j%NUMBER_OF_PROCESSORS%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
echo.

REM Package
echo [STEP 4/5] Creating package...
cd ..

if not exist dist mkdir dist
if not exist package\bin mkdir package\bin
if not exist package\share\doc\encfsmp-cli mkdir package\share\doc\encfsmp-cli

REM Find and copy the executable
set EXE_FOUND=0
for /r build %%F in (encfsmp_cli.exe) do (
    if exist "%%F" (
        copy "%%F" package\bin\ /Y >nul
        set EXE_FOUND=1
        echo [INFO] Found: %%F
    )
)

if %EXE_FOUND% equ 0 (
    echo [ERROR] Executable not found!
    pause
    exit /b 1
)

REM Copy documentation
if exist README.md (
    copy README.md package\ /Y >nul
    copy README.md package\share\doc\encfsmp-cli\ /Y >nul
)
if exist cheatsheet.txt (
    copy cheatsheet.txt package\share\doc\encfsmp-cli\ /Y >nul
)

REM Strip debug info
strip package\bin\encfsmp_cli.exe 2>nul

REM Create ZIP
echo [STEP 5/5] Creating ZIP archive...
powershell -Command "Compress-Archive -Path package\* -DestinationPath dist\encfsmp-cli-win-x64.zip -Force"

echo.
echo ================================================
echo  Build Complete!
echo ================================================
echo.
echo Output files:
dir /b dist\*.zip 2>nul
echo.
dir /b package\bin\*.exe 2>nul
echo.
echo To download, use the file: dist\encfsmp-cli-win-x64.zip
echo.
echo Usage on Windows:
echo   .\package\bin\encfsmp_cli.exe --help
echo   .\package\bin\encfsmp_cli.exe create vault -p password
echo.
pause
