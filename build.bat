@echo off
rem ---------------------------------------------------------------------------
rem ChronoSort のビルド。
rem   build.bat            … Debug をビルド
rem   build.bat Release    … Release をビルド
rem
rem 成功時の出力は 0 行、失敗時はエラー行だけになる。
rem MSBuild の探し方はエンジンの build.bat と同じ（理由はそちらのコメント参照）。
rem ---------------------------------------------------------------------------
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "ROOT=%~dp0"
set "SLN=%ROOT%.build\ChronoSort.sln"
set "PREMAKE=%ROOT%External\TsukinoEngine\vendor\premake5.exe"

set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" call :find_vs2022
if not defined MSBUILD if exist "%VSWHERE%" call :find_latest
if not defined MSBUILD set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

if not exist "%MSBUILD%" (
    echo build.bat: MSBuild not found. Install Visual Studio 2022 with the "Desktop development with C++" workload.
    exit /b 1
)

rem ソースファイルを追加したときも再生成が要るため、毎回 premake を通す
rem （中身が同じならプロジェクトファイルは書き換わらず、再ビルドも起きない）
rem エンジン側 premake5.lua の非推奨警告が毎回出るため、成功時は出力を捨てる
"%PREMAKE%" vs2022 >nul 2>&1
if errorlevel 1 (
    echo build.bat: premake failed. Run open.bat to see the details.
    exit /b 1
)

rem NuGet の復元。premake が吐くのは旧形式の packages.config なので
rem -p:RestorePackagesConfig=true が要る
if not exist "%ROOT%.build\packages" if not exist "%ROOT%packages" (
    "%MSBUILD%" "%SLN%" -t:restore -p:RestorePackagesConfig=true -p:Configuration=%CONFIG% -p:Platform=x64 -nologo -v:q -clp:"ErrorsOnly;NoSummary"
    if errorlevel 1 exit /b 1
)

"%MSBUILD%" "%SLN%" /p:Configuration=%CONFIG% /p:Platform=x64 /m /nologo /v:q /clp:"ErrorsOnly;NoSummary"
exit /b %ERRORLEVEL%

:find_vs2022
for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -property installationPath^"`) do (
    if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%i\MSBuild\Current\Bin\MSBuild.exe"
)
goto :eof

:find_latest
for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath^"`) do (
    if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%i\MSBuild\Current\Bin\MSBuild.exe"
)
goto :eof
