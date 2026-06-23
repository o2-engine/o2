@echo off
setlocal
cd /d "%~dp0"

if not exist .env (
  copy .env.example .env >nul
  echo Created .env from .env.example
)

where docker >nul 2>nul
if errorlevel 1 (
  echo ERROR: docker is not on PATH.
  echo Install Docker Desktop and make sure it is running, then re-run start.bat.
  pause
  exit /b 1
)

echo Building and starting containers...
docker compose up -d --build
set RC=%errorlevel%
if not "%RC%"=="0" (
  echo Trying legacy docker-compose...
  docker-compose up -d --build
  set RC=%errorlevel%
)

if not "%RC%"=="0" (
  echo.
  echo ERROR: docker compose failed with exit code %RC%.
  echo Verify that Docker Desktop is running (whale icon in tray).
  echo Run "docker compose up --build" manually here to see the full error.
  pause
  exit /b %RC%
)

echo.
echo Waiting for services to come up...
timeout /t 4 /nobreak >nul
start "" "http://localhost:5173"

echo.
echo AssetGenerator is running.
echo   Frontend: http://localhost:5173
echo   Backend:  http://localhost:8765
echo Stop with: stop.bat
echo.
pause
