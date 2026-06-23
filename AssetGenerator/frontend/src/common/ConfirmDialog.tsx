import React from 'react';

interface Props {
  title: string;
  message: string;
  confirmLabel?: string;
  cancelLabel?: string;
  destructive?: boolean;
  onConfirm(): void;
  onCancel(): void;
}

export function ConfirmDialog({ title, message, confirmLabel = 'OK', cancelLabel = 'Cancel', destructive, onConfirm, onCancel }: Props): JSX.Element {
  return (
    <div className="dialog-backdrop" onMouseDown={(e) => { if (e.target === e.currentTarget) onCancel(); }}>
      <div className="dialog" style={{ minWidth: 360 }}>
        <h2>{title}</h2>
        <div style={{ marginBottom: 14, whiteSpace: 'pre-wrap' }}>{message}</div>
        <div className="dialog-actions">
          <button onClick={onCancel}>{cancelLabel}</button>
          <button onClick={onConfirm} className={destructive ? 'btn-danger' : 'btn-primary'}>{confirmLabel}</button>
        </div>
      </div>
    </div>
  );
}
