const row = (r) => (r ? { ...r, style: JSON.parse(r.style), worn: JSON.parse(r.worn), visible: !!r.visible } : null);

const limiters = new WeakMap();

export function d1Store(env) {
  const db = env.DB;
  if (!limiters.has(db)) limiters.set(db, new Map());
  const counts = limiters.get(db);
  return {
    async player(key) {
      return row(await db.prepare('SELECT * FROM players WHERE key = ?').bind(key).first());
    },
    async savePlayer(p) {
      await db
        .prepare(
          `INSERT INTO players (key, name, secret, style, worn, visible, server, client, seen, created)
           VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
           ON CONFLICT(key) DO UPDATE SET name = excluded.name, secret = excluded.secret, style = excluded.style, worn = excluded.worn,
             visible = excluded.visible, server = excluded.server, client = excluded.client, seen = excluded.seen`,
        )
        .bind(p.key, p.name, p.secret, JSON.stringify(p.style), JSON.stringify(p.worn), p.visible ? 1 : 0, p.server, p.client, p.seen, p.created)
        .run();
    },
    async removePlayer(key) {
      await db.batch([db.prepare('DELETE FROM sessions WHERE key = ?').bind(key), db.prepare('DELETE FROM players WHERE key = ?').bind(key)]);
    },
    async session(token) {
      return db.prepare('SELECT key, expires FROM sessions WHERE token = ?').bind(token).first();
    },
    async saveSession(token, key, expires) {
      await db.batch([
        db.prepare('INSERT OR REPLACE INTO sessions (token, key, expires) VALUES (?, ?, ?)').bind(token, key, expires),
        db
          .prepare('DELETE FROM sessions WHERE key = ? AND token NOT IN (SELECT token FROM sessions WHERE key = ? ORDER BY expires DESC LIMIT 3)')
          .bind(key, key),
      ]);
    },
    async removeSession(token) {
      await db.prepare('DELETE FROM sessions WHERE token = ?').bind(token).run();
    },
    async lookup(keys, since) {
      if (!keys.length) return [];
      const marks = keys.map(() => '?').join(',');
      const { results } = await db
        .prepare(`SELECT * FROM players WHERE key IN (${marks}) AND visible = 1 AND seen >= ?`)
        .bind(...keys, since)
        .all();
      return results.map(row);
    },
    async count(since) {
      const r = await db.prepare('SELECT COUNT(*) AS n FROM players WHERE visible = 1 AND seen >= ?').bind(since).first();
      return r ? r.n : 0;
    },
    async isBlocked(key) {
      return !!(await db.prepare('SELECT 1 AS x FROM blocked WHERE key = ?').bind(key).first());
    },
    async block(key, reason) {
      await db.batch([
        db.prepare('INSERT OR REPLACE INTO blocked (key, reason) VALUES (?, ?)').bind(key, reason),
        db.prepare('DELETE FROM sessions WHERE key = ?').bind(key),
        db.prepare('DELETE FROM players WHERE key = ?').bind(key),
      ]);
    },
    async hit(bucket, limit, windowSec, now) {
      const slot = `${bucket}:${Math.floor(now / windowSec)}`;
      const n = (counts.get(slot) ?? 0) + 1;
      counts.set(slot, n);
      if (counts.size > 5000) counts.clear();
      return n <= limit;
    },
    async sweep(before) {
      await db.batch([
        db.prepare('DELETE FROM players WHERE seen < ?').bind(before),
        db.prepare('DELETE FROM sessions WHERE expires < ?').bind(before),
      ]);
    },
  };
}
