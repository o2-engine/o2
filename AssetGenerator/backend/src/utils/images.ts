import sharp from 'sharp';

export async function removeBackgroundTwoPass(whitePng: Buffer, blackPng: Buffer): Promise<Buffer> {
  const w = await sharp(whitePng).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
  const b = await sharp(blackPng).ensureAlpha().raw().toBuffer({ resolveWithObject: true });

  if (w.info.width !== b.info.width || w.info.height !== b.info.height) {
    throw new Error(`Dimension mismatch: white=${w.info.width}x${w.info.height}, black=${b.info.width}x${b.info.height}`);
  }

  const { width, height } = w.info;
  const wb = w.data;
  const bb = b.data;
  const out = Buffer.alloc(width * height * 4);

  const bgDist = Math.sqrt(3 * 255 * 255);

  for (let i = 0; i < width * height; i++) {
    const idx = i * 4;
    const rw = wb[idx];
    const gw = wb[idx + 1];
    const bw = wb[idx + 2];
    const rb = bb[idx];
    const gb = bb[idx + 1];
    const bb_ = bb[idx + 2];

    const dr = rw - rb;
    const dg = gw - gb;
    const db = bw - bb_;
    const pixelDist = Math.sqrt(dr * dr + dg * dg + db * db);

    let alpha = 1 - pixelDist / bgDist;
    if (alpha < 0) alpha = 0;
    else if (alpha > 1) alpha = 1;

    if (alpha > 0.01) {
      const inv = 1 / alpha;
      let r = rb * inv;
      let g = gb * inv;
      let bch = bb_ * inv;
      if (r > 255) r = 255; else if (r < 0) r = 0;
      if (g > 255) g = 255; else if (g < 0) g = 0;
      if (bch > 255) bch = 255; else if (bch < 0) bch = 0;
      out[idx] = Math.round(r);
      out[idx + 1] = Math.round(g);
      out[idx + 2] = Math.round(bch);
    } else {
      out[idx] = 0;
      out[idx + 1] = 0;
      out[idx + 2] = 0;
    }
    out[idx + 3] = Math.round(alpha * 255);
  }

  return sharp(out, { raw: { width, height, channels: 4 } }).png().toBuffer();
}
