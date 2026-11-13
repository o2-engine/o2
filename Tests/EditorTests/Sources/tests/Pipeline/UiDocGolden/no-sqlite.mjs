// Loads cases4.mts without the backend's billing database: better-sqlite3 (a Linux build on this host) becomes a stub
import { register } from 'node:module';
register('data:text/javascript,' + encodeURIComponent(`
export async function resolve(specifier, context, next) {
  if (specifier === 'better-sqlite3') return { url: 'data:text/javascript,' + encodeURIComponent('const any = () => new Proxy(function () {}, { get: (t, k) => k === Symbol.toPrimitive ? () => 0 : k === "then" ? undefined : any(), apply: () => any(), construct: () => any() }); export default any();'), shortCircuit: true };
  return next(specifier, context);
}`));
