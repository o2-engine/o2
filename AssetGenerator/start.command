#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
[ -f .env ] || cp .env.example .env
docker compose up -d --build
sleep 3
URL="http://localhost:5173"
(open "$URL" 2>/dev/null) || (xdg-open "$URL" 2>/dev/null) || echo "Open $URL in your browser"
echo ""
echo "AssetGenerator is running."
echo "  Frontend: http://localhost:5173"
echo "  Backend:  http://localhost:8765"
echo "Stop with: ./stop.command"
