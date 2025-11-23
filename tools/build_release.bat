@echo off
chcp 65001 > nul

REM ============================================
REM リリースバージョン
REM ============================================
set VERSION=1.0.0

REM ============================================
REM 現在日時を取得（YYYYMMDDhhmmss形式）
REM ============================================
set NOW=%DATE:~0,4%%DATE:~5,2%%DATE:~8,2%%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%
set NOW=%NOW: =0%

REM ============================================
REM パス定義（tools フォルダ基準）
REM ============================================
set ROOT_DIR=%~dp0..
set SERVER_DIR=%ROOT_DIR%\server
set ANGULAR_DIR=%ROOT_DIR%
set DIST_DIR=%ANGULAR_DIR%\dist\sales-serch\browser
set RELEASE_BASE=%ROOT_DIR%\Release
set RELEASE_DIR=%RELEASE_BASE%\%NOW%

echo.
echo ================================
echo  リリースビルド開始 v%VERSION%
echo  出力先: %RELEASE_DIR%
echo ================================

REM ============================================
REM Angular Production Build
REM ============================================
echo Angular を production ビルドします...
cd /d %ANGULAR_DIR%
cmd /c "npx ng build --configuration production"

if %ERRORLEVEL% neq 0 (
  echo Angular ビルドに失敗しました。
  pause
  exit /b 1
)

REM ============================================
REM 日時フォルダ作成
REM ============================================
echo リリース用フォルダを作成...
mkdir "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%\browser"

REM ============================================
REM Angular成果物コピー
REM ============================================
echo Angular ビルド成果物をコピー...
xcopy "%DIST_DIR%\*" "%RELEASE_DIR%\browser\" /E /H /C /I /Y

REM ============================================
REM C サーバー EXE ビルド
REM ============================================
echo Cサーバーをビルド中...
cd /d %SERVER_DIR%

gcc main.c mongoose.c sqlite3.c -o "%RELEASE_DIR%\NF_Db_server_v%VERSION%.exe" -lws2_32

if %ERRORLEVEL% neq 0 (
  echo サーバーEXEの生成に失敗しました。
  pause
  exit /b 1
)

REM ============================================
REM DB コピー
REM ============================================
echo DBコピー...
copy "%SERVER_DIR%\sales.db" "%RELEASE_DIR%\" /Y

echo.
echo ✅ リリースビルド完了！
echo --------------------------------
echo フォルダ : %RELEASE_DIR%
echo EXE      : NF_Db_server_v%VERSION%.exe
echo UI       : browser フォルダ
echo DB       : sales.db
echo --------------------------------

pause
