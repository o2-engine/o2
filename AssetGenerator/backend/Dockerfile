FROM node:20-bookworm-slim

WORKDIR /app/backend

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY backend/package.json ./
RUN npm install --no-audit --no-fund

COPY backend/tsconfig.json ./
COPY backend/src ./src
COPY shared /app/shared

EXPOSE 8765

CMD ["npx", "tsx", "watch", "src/index.ts"]
