import React, { useEffect, useState } from 'react';

interface Props {
  root: 'assets' | 'contentDb';
  path: string;
  maxSize?: number;
  className?: string;
  fill?: boolean;
}

export function imagePreviewUrl(root: 'assets' | 'contentDb', path: string): string {
  return `/api/files/read?root=${encodeURIComponent(root)}&path=${encodeURIComponent(path)}`;
}

export function ImagePreview({ root, path, maxSize = 96, className, fill }: Props): JSX.Element {
  const [status, setStatus] = useState<'loading' | 'ok' | 'failed' | 'empty'>(path.trim() ? 'loading' : 'empty');

  useEffect(() => {
    if (!path.trim()) { setStatus('empty'); return; }
    setStatus('loading');
    const url = imagePreviewUrl(root, path);
    const img = new Image();
    img.onload = () => setStatus('ok');
    img.onerror = () => setStatus('failed');
    img.src = url;
    return () => { img.onload = null; img.onerror = null; };
  }, [root, path]);

  const sizeStyle: React.CSSProperties = fill
    ? { width: '100%', height: '100%' }
    : { width: maxSize, height: maxSize };

  if (status === 'empty' || status === 'failed') {
    return (
      <div className={`image-preview empty ${className ?? ''}`} style={sizeStyle} title={status === 'failed' ? `Failed to load ${path}` : 'No image'}>
        {status === 'failed' ? '!' : '—'}
      </div>
    );
  }

  return (
    <div
      className={`image-preview ${className ?? ''}`}
      style={{
        ...sizeStyle,
        backgroundImage: `url("${imagePreviewUrl(root, path)}")`,
        backgroundSize: 'contain',
        backgroundRepeat: 'no-repeat',
        backgroundPosition: 'center',
      }}
      title={path}
    />
  );
}
