export async function systemSaveBlob(blob: Blob, suggestedName: string, mimeAccept?: string): Promise<boolean> {
  const w = window as any;
  if (typeof w.showSaveFilePicker === 'function') {
    try {
      const handle = await w.showSaveFilePicker({
        suggestedName,
        types: mimeAccept ? [{ description: 'File', accept: { [mimeAccept]: [`.${suggestedName.split('.').pop() ?? 'bin'}`] } }] : undefined,
      });
      const writable = await handle.createWritable();
      await writable.write(blob);
      await writable.close();
      return true;
    } catch (e: any) {
      if (e?.name === 'AbortError') return false;
      throw e;
    }
  }
  // Fallback: anchor download to user's default downloads folder
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = suggestedName;
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
  return true;
}

export async function systemSaveFromUrl(url: string, suggestedName: string, mimeAccept?: string): Promise<boolean> {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`Fetch failed: HTTP ${res.status}`);
  const blob = await res.blob();
  return systemSaveBlob(blob, suggestedName, mimeAccept);
}
