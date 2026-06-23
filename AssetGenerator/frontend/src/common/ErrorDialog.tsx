import React from 'react';
import { useErrorDialogStore } from './errorDialogStore.js';
import { AlertIcon } from '../tools/nodeEditor/icons.js';

export function ErrorDialog(): JSX.Element | null {
  const queue = useErrorDialogStore((s) => s.queue);
  const dismiss = useErrorDialogStore((s) => s.dismiss);
  const clear = useErrorDialogStore((s) => s.clear);
  const current = queue[0];
  if (!current) return null;

  return (
    <div className="dialog-backdrop" onMouseDown={(e) => { if (e.target === e.currentTarget) dismiss(current.id); }}>
      <div className="dialog" style={{ minWidth: 420, maxWidth: 640 }}>
        <h2 style={{ display: 'flex', alignItems: 'center', gap: 8, color: 'var(--red)' }}>
          <AlertIcon size={18} color="var(--red)" /> Error
        </h2>
        <div style={{ marginBottom: 12, whiteSpace: 'pre-wrap' }}>{current.message}</div>
        {current.details && (
          <>
            <div className="dialog-section">Details</div>
            <pre style={{
              background: 'var(--bg)',
              border: '1px solid var(--border)',
              borderRadius: 3,
              padding: 8,
              whiteSpace: 'pre-wrap',
              wordBreak: 'break-word',
              maxHeight: 320,
              overflow: 'auto',
              fontSize: 11,
              margin: 0,
            }}>{current.details}</pre>
          </>
        )}
        <div style={{ color: 'var(--fg-dim)', fontSize: 11, marginTop: 8 }}>
          {new Date(current.ts).toLocaleString()}
          {queue.length > 1 && ` · ${queue.length - 1} more queued`}
        </div>
        <div className="dialog-actions">
          {queue.length > 1 && <button onClick={clear}>Dismiss all</button>}
          <button onClick={() => dismiss(current.id)} className="btn-primary">Close</button>
        </div>
      </div>
    </div>
  );
}
