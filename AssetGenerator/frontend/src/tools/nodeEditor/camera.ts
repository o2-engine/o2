import { MAX_SCALE, MIN_SCALE } from './constants.js';

export interface Camera {
  x: number;
  y: number;
  scale: number;
}

export function screenToWorld(c: Camera, sx: number, sy: number): { x: number; y: number } {
  return { x: (sx - c.x) / c.scale, y: (sy - c.y) / c.scale };
}

export function zoomAt(c: Camera, sx: number, sy: number, factor: number): Camera {
  const newScale = Math.min(MAX_SCALE, Math.max(MIN_SCALE, c.scale * factor));
  const k = newScale / c.scale;
  return {
    scale: newScale,
    x: sx - (sx - c.x) * k,
    y: sy - (sy - c.y) * k,
  };
}
