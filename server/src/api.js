import { cleanServer, cleanStyle, cleanWorn, keyOf, nameOk, roleOk } from './validate.js';

const online = 300;
const sessionTtl = 12 * 3600;
const reclaimAfter = 30 * 86400;
const maxBody = 16 * 1024;

const json = (data, status = 200) =>
  new Response(JSON.stringify(data), { status, headers: { 'content-type': 'application/json', 'cache-control': 'no-store' } });

const fail = (status, error) => json({ error }, status);

const hexOf = (bytes) => [...new Uint8Array(bytes)].map((b) => b.toString(16).padStart(2, '0')).join('');

async function sha256(text) {
  return hexOf(await crypto.subtle.digest('SHA-256', new TextEncoder().encode(text)));
}

function randomToken() {
  return hexOf(crypto.getRandomValues(new Uint8Array(32)));
}

async function readBody(request) {
  const text = await request.text();
  if (text.length > maxBody) return { error: 413 };
  if (!text) return { body: {} };
  try {
    const body = JSON.parse(text);
    return body && typeof body === 'object' ? { body } : { error: 400 };
  } catch {
    return { error: 400 };
  }
}

async function authed(request, store, now) {
  const header = request.headers.get('authorization') ?? '';
  const token = header.startsWith('Bearer ') ? header.slice(7).trim() : '';
  if (!/^[0-9a-f]{64}$/.test(token)) return null;
  const hash = await sha256(token);
  const session = await store.session(hash);
  if (!session || session.expires < now) return null;
  const player = await store.player(session.key);
  if (!player) return null;
  if (session.expires - now < sessionTtl / 2) await store.saveSession(hash, session.key, now + sessionTtl);
  return { player, hash };
}

async function hello(body, ip, store, now) {
  if (!(await store.hit(`hello:${ip}`, 10, 60, now))) return fail(429, 'slow down');
  if (!nameOk(body.name)) return fail(400, 'name');
  if (typeof body.secret !== 'string' || !/^[0-9a-f]{48}$/.test(body.secret)) return fail(400, 'secret');
  const key = keyOf(body.name);
  if (await store.isBlocked(key)) return fail(403, 'blocked');

  const secret = await sha256(body.secret);
  const existing = await store.player(key);
  if (existing && existing.secret !== secret && now - existing.seen < reclaimAfter) return fail(403, 'claimed');

  const style = cleanStyle(body.style);
  const same = existing && existing.secret === secret;
  const player = {
    key,
    name: body.name,
    secret,
    style,
    worn: cleanWorn(body.worn),
    visible: body.visible !== false,
    server: '',
    client: typeof body.client === 'string' ? body.client.slice(0, 16) : '',
    seen: now,
    created: same ? existing.created : now,
    role: same ? existing.role ?? '' : '',
  };
  await store.savePlayer(player);

  const token = randomToken();
  await store.saveSession(await sha256(token), key, now + sessionTtl);
  return json({ token, ttl: sessionTtl, style, worn: player.worn, role: player.role });
}

async function profile(body, auth, store, now) {
  const style = cleanStyle(body.style ?? auth.player.style);
  const player = {
    ...auth.player,
    style,
    worn: body.worn === undefined ? auth.player.worn : cleanWorn(body.worn),
    visible: body.visible === undefined ? auth.player.visible : body.visible !== false,
    seen: now,
  };
  await store.savePlayer(player);
  return json({ style, worn: player.worn });
}

async function presence(body, auth, store, now) {
  await store.savePlayer({ ...auth.player, server: cleanServer(body.server), seen: now });
  return json({ ok: true });
}

async function lookup(body, auth, store, now) {
  const names = Array.isArray(body.names) ? body.names.filter(nameOk).slice(0, 100) : [];
  const keys = [...new Set(names.map(keyOf))];
  const found = await store.lookup(keys, now - online);
  const users = found.map((p) => ({ name: p.name, style: p.style, worn: p.worn, role: p.role ?? '' }));
  return json({ users });
}

async function bye(auth, store, now) {
  await store.removeSession(auth.hash);
  await store.savePlayer({ ...auth.player, server: '', seen: Math.min(auth.player.seen, now - online - 1) });
  return json({ ok: true });
}

async function forget(auth, store) {
  await store.removePlayer(auth.player.key);
  return json({ ok: true });
}

async function isAdmin(request, env) {
  const given = request.headers.get('x-admin-key') ?? '';
  return !!env.ADMIN_KEY && given.length === env.ADMIN_KEY.length && (await sha256(given)) === (await sha256(env.ADMIN_KEY));
}

async function admin(request, body, store, env) {
  if (!(await isAdmin(request, env))) return fail(403, 'no');
  if (!nameOk(body.name)) return fail(400, 'name');
  await store.block(keyOf(body.name), typeof body.reason === 'string' ? body.reason.slice(0, 200) : '');
  return json({ ok: true });
}

// A role sticks to the install that holds the gamertag: when someone else reclaims it, hello drops the role.
async function setRole(request, body, store, env) {
  if (!(await isAdmin(request, env))) return fail(403, 'no');
  if (!nameOk(body.name)) return fail(400, 'name');
  const role = body.role ?? '';
  if (!roleOk(role)) return fail(400, 'role');
  const player = await store.player(keyOf(body.name));
  if (!player) return fail(404, 'unknown');
  await store.savePlayer({ ...player, role });
  return json({ ok: true, name: player.name, role });
}

export async function handle(request, env, store, now = Math.floor(Date.now() / 1000)) {
  const url = new URL(request.url);
  const path = url.pathname.replace(/\/+$/, '');

  if (path === '/v1/health') return json({ ok: true, online: await store.count(now - online) });
  if (request.method !== 'POST') return fail(405, 'method');

  const read = await readBody(request);
  if (read.error) return fail(read.error, 'body');
  const body = read.body;
  const ip = request.headers.get('cf-connecting-ip') ?? 'local';

  if (path === '/v1/hello') return hello(body, ip, store, now);
  if (path === '/v1/admin/block') return admin(request, body, store, env);
  if (path === '/v1/admin/role') return setRole(request, body, store, env);

  const auth = await authed(request, store, now);
  if (!auth) return fail(401, 'token');
  if (!(await store.hit(`${path}:${auth.player.key}`, 30, 60, now))) return fail(429, 'slow down');

  switch (path) {
    case '/v1/profile':
      return profile(body, auth, store, now);
    case '/v1/presence':
      return presence(body, auth, store, now);
    case '/v1/lookup':
      return lookup(body, auth, store, now);
    case '/v1/bye':
      return bye(auth, store, now);
    case '/v1/forget':
      return forget(auth, store);
    default:
      return fail(404, 'route');
  }
}

export async function sweep(store, now = Math.floor(Date.now() / 1000)) {
  await store.sweep(now - 90 * 86400);
}
