FROM node:20-bookworm-slim

WORKDIR /app/frontend

COPY frontend/package.json ./
RUN npm install --no-audit --no-fund

COPY frontend/tsconfig.json frontend/vite.config.ts frontend/index.html ./
COPY frontend/src ./src
COPY shared /app/shared

EXPOSE 5173

CMD ["npm", "run", "dev"]
