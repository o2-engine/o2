import React, { useState } from 'react';
import { useToastStore, type Toast } from './toastStore.js';
import { AlertIcon, CloseIcon, InfoIcon } from '../tools/nodeEditor/icons.js';

export function ToastContainer(): JSX.Element {
  const toasts = useToastStore((s) => s.toasts);
  const dismiss = useToastStore((s) => s.dismiss);
  const [openToast, setOpenToast] = useState<Toast | null>(null);

  return (
    <>
      <div className="toast-stack">
        {toasts.slice().reverse().map((t) => (
          <button
            key={t.id}
            className={`toast toast-${t.level}`}
            onClick={() => setOpenToast(t)}
            title="Click to see details"
          >
            <span className="toast-icon">
              {t.level === 'error' ? <AlertIcon size={14} /> : <InfoIcon size={14} />}
            </span>
            <span className="toast-message">{t.message}</span>
            <span
              className="toast-close"
              onClick={(e) => { e.stopPropagation(); dismiss(t.id); }}
              title="Dismiss"
            >
              <CloseIcon size={12} />
            </span>
          </button>
        ))}
      </div>

      {openToast && (
        <div className="dialog-backdrop" onMouseDown={(e) => { if (e.target === e.currentTarget) setOpenToast(null); }}>
          <div className="dialog">
            <h2>
              <span className="toast-icon">
                {openToast.level === 'error' ? <AlertIcon size={16} /> : <InfoIcon size={16} />}
              </span>
              {' '}
              {openToast.level === 'error' ? 'Error' : 'Info'}
            </h2>
            <div style={{ marginBottom: 12, whiteSpace: 'pre-wrap' }}>{openToast.message}</div>
            {openToast.details && (
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
                }}>{openToast.details}</pre>
              </>
            )}
            <div style={{ color: 'var(--fg-dim)', fontSize: 11, marginTop: 8 }}>
              {new Date(openToast.ts).toLocaleString()}
            </div>
            <div className="dialog-actions">
              <button onClick={() => { dismiss(openToast.id); setOpenToast(null); }}>Dismiss</button>
              <button onClick={() => setOpenToast(null)}>Close</button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}
