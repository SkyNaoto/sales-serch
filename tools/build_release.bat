@echo off
chcp 65001 > nul

REM ============================================
REM リリースバージョン
REM ============================================
set VERSION=1.0.0

REM ============================================
REM パス定義（tools 基準）
REM ============================================
set ROOT_DIR=%~dp0..
set SERVER_DIR=%ROOT_DIR%\server
set ANGULAR_DIR=%ROOT_DIR%
set DIST_DIR=%ANGULAR_DIR%\dist\sales-serch\browser
set RELEASE_DIR=%ROOT_DIR%\Release

echo.
echo ================================
echo  リリースビルド開始 v%VERSION%
echo ================================

REM ============================================
REM Angular Production Build
REM ============================================
echo Angular を production ビルドします...
cd /d "%ANGULAR_DIR%"
call npx ng build --configuration production

if %ERRORLEVEL% neq 0 (
  echo ❌ Angular ビルドに失敗しました。
  pause
  exit /b 1
)

REM ============================================
REM Release フォルダ作成
REM ============================================
echo Release フォルダを作成...
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"
if not exist "%RELEASE_DIR%\browser" mkdir "%RELEASE_DIR%\browser"

REM ============================================
REM Angular 成果物コピー
REM ============================================
echo Angular ビルド成果物をコピー...
xcopy "%DIST_DIR%\*" "%RELEASE_DIR%\browser\" /E /H /C /I /Y > nul

REM ============================================
REM C サーバー EXE ビルド
REM ============================================
echo Cサーバーをビルド中...
cd /d "%SERVER_DIR%"

gcc main.c mongoose.c sqlite3.c -O2 -o "%RELEASE_DIR%\NF_Db_server_v%VERSION%.exe" -lws2_32

if %ERRORLEVEL% neq 0 (
  echo ❌ サーバーEXEの生成に失敗しました。
  pause
  exit /b 1
)

REM ============================================
REM DB コピー
REM ============================================
echo DBコピー...
copy "%SERVER_DIR%\sales.db" "%RELEASE_DIR%\" /Y > nul

echo.
echo ✅ リリースビルド完了！
echo --------------------------------
echo EXE : NF_Db_server_v%VERSION%.exe
echo UI  : browser フォルダ
echo DB  : sales.db
echo --------------------------------

explorer "%RELEASE_DIR%"

pause
