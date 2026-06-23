@echo off
setlocal
cd /d "%~dp0"

where docker >nul 2>nul
if errorlevel 1 (
  echo ERROR: docker is not on PATH.
  pause
  exit /b 1
)

echo Stopping containers...
docker compose down
if errorlevel 1 (
  docker-compose down
)

echo Done.
pause
