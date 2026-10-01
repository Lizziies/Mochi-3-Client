import assert from 'node:assert/strict';
import { beforeEach, test } from 'node:test';
import { handle } from '../src/api.js';
import { memoryStore } from '../src/store_memory.js';
import { d1Store } from '../src/store_d1.js';
import { fakeEnv } from './d1shim.js';

const backend = process.env.STORE === 'd1' ? 'd1' : 'memory';
let store;
let clock;
const env = { ADMIN_KEY: 'letmein' };
const secretA = 'a'.repeat(48);
const secretB = 'b'.repeat(48);

beforeEach(() => {
  store = backend === 'd1' ? d1Store(fakeEnv()) : memoryStore();
  clock = 1_800_000_000;
});

async function call(path, body = {}, token = '', extra = {}) {
  const headers = { 'content-type': 'application/json', 'cf-connecting-ip': extra.ip ?? '1.1.1.1', ...(extra.headers ?? {}) };
  if (token) headers.authorization = `Bearer ${token}`;
  const res = await handle(new Request(`http://x${path}`, { method: 'POST', headers, body: JSON.stringify(body) }), env, store, clock);
  return { status: res.status, data: await res.json() };
}

async function login(name, secret = secretA, extra = {}) {
  const r = await call('/v1/hello', { name, secret, client: '0.1.0', ...extra });
  assert.equal(r.status, 200, JSON.stringify(r.data));
  return r.data.token;
}

test('hello returns a token and cleaned style', async () => {
  const r = await call('/v1/hello', { name: 'Luna', secret: secretA, style: { mode: 'rainbow', a: '#FF0000', speed: 99, tag: 'Pro§k' } });
  assert.equal(r.status, 200);
  assert.match(r.data.token, /^[0-9a-f]{64}$/);
  assert.equal(r.data.style.mode, 'rainbow');
  assert.equal(r.data.style.a, '#ff0000');
  assert.equal(r.data.style.speed, 5);
  assert.equal(r.data.style.heartColor, '#ff3b5c');
  assert.equal(r.data.style.tag, undefined);
});

test('bad names and secrets are rejected', async () => {
  assert.equal((await call('/v1/hello', { name: '', secret: secretA })).status, 400);
  assert.equal((await call('/v1/hello', { name: 'x<script>', secret: secretA })).status, 400);
  assert.equal((await call('/v1/hello', { name: 'Luna', secret: 'short' })).status, 400);
});

test('a claimed gamertag needs the same secret', async () => {
  await login('Luna', secretA);
  const stolen = await call('/v1/hello', { name: 'luna', secret: secretB });
  assert.equal(stolen.status, 403);
  assert.equal(stolen.data.error, 'claimed');
  await login('LUNA', secretA);
});

test('an abandoned gamertag can be reclaimed after 30 days', async () => {
  await login('Luna', secretA);
  clock += 31 * 86400;
  await login('Luna', secretB);
});

test('lookup returns only visible users that were seen recently', async () => {
  const luna = await login('Luna');
  await login('Kiki', secretB, { style: { heartColor: '#00ff00', heart: false } });
  await login('Hidden', 'c'.repeat(48), { visible: false });
  const r = await call('/v1/lookup', { names: ['kiki', 'Hidden', 'Nobody', 'Luna'] }, luna);
  assert.equal(r.status, 200);
  assert.deepEqual(r.data.users.map((u) => u.name).sort(), ['Kiki', 'Luna']);
  assert.equal(r.data.users.find((u) => u.name === 'Kiki').style.heart, false);
  assert.equal(r.data.users.find((u) => u.name === 'Kiki').style.heartColor, '#00ff00');
  assert.equal(r.data.online, 2);

  clock += 400;
  const later = await call('/v1/lookup', { names: ['Kiki', 'Luna'] }, luna);
  assert.deepEqual(later.data.users.map((u) => u.name), ['Luna']);
});

test('lookup keeps seen fresh and drops silent users', async () => {
  const luna = await login('Luna');
  await login('Kiki', secretB);
  clock += 100;
  await call('/v1/lookup', { names: [] }, luna);
  clock += 100;
  const r = await call('/v1/lookup', { names: ['Kiki', 'Luna'] }, luna);
  assert.deepEqual(r.data.users.map((u) => u.name), ['Luna']);
});

test('profile updates style and worn items', async () => {
  const token = await login('Luna');
  const r = await call('/v1/profile', { style: { mode: 'pulse', tag: 'free text is ignored' }, worn: [{ id: 'sakura_wings', tint: ['#ff7eb6', 'bad'] }, { id: 'No Good' }] }, token);
  assert.equal(r.status, 200);
  assert.equal(r.data.style.mode, 'pulse');
  assert.equal(r.data.style.tag, undefined);
  assert.deepEqual(r.data.worn, [{ id: 'sakura_wings', tint: ['#ff7eb6', '#ffffff'] }]);
});

test('worn is limited to 8 items', async () => {
  const token = await login('Luna');
  const worn = Array.from({ length: 20 }, (_, i) => ({ id: `item_${i}` }));
  const r = await call('/v1/profile', { worn }, token);
  assert.equal(r.data.worn.length, 8);
});

test('calls without a valid token get 401', async () => {
  assert.equal((await call('/v1/lookup', { names: ['Luna'] })).status, 401);
  assert.equal((await call('/v1/lookup', { names: ['Luna'] }, 'f'.repeat(64))).status, 401);
});

test('bye ends the session and hides the user', async () => {
  const luna = await login('Luna');
  const kiki = await login('Kiki', secretB);
  assert.equal((await call('/v1/bye', {}, kiki)).status, 200);
  assert.equal((await call('/v1/lookup', {}, kiki)).status, 401);
  const r = await call('/v1/lookup', { names: ['Kiki'] }, luna);
  assert.deepEqual(r.data.users, []);
});

test('forget deletes the player', async () => {
  const luna = await login('Luna');
  assert.equal((await call('/v1/forget', {}, luna)).status, 200);
  assert.equal((await call('/v1/lookup', {}, luna)).status, 401);
  await login('Luna', secretB);
});

test('presence stores a cleaned server name', async () => {
  const luna = await login('Luna');
  assert.equal((await call('/v1/presence', { server: 'The Hive' }, luna)).status, 200);
  assert.equal((await store.player('luna')).server, 'The Hive');
});

test('hello is rate limited per address', async () => {
  for (let i = 0; i < 10; i++) await call('/v1/hello', { name: `P${i}`, secret: secretA });
  assert.equal((await call('/v1/hello', { name: 'P99', secret: secretA })).status, 429);
  assert.equal((await call('/v1/hello', { name: 'P99', secret: secretA }, '', { ip: '2.2.2.2' })).status, 200);
});

test('calls with a token are rate limited per route', async () => {
  const token = await login('Luna');
  for (let i = 0; i < 30; i++) assert.equal((await call('/v1/lookup', {}, token)).status, 200);
  assert.equal((await call('/v1/lookup', {}, token)).status, 429);
});

test('admin can block a gamertag', async () => {
  const luna = await login('Luna');
  const denied = await call('/v1/admin/block', { name: 'Luna' }, '', { headers: { 'x-admin-key': 'wrong' } });
  assert.equal(denied.status, 403);
  const ok = await call('/v1/admin/block', { name: 'Luna', reason: 'test' }, '', { headers: { 'x-admin-key': 'letmein' } });
  assert.equal(ok.status, 200);
  assert.equal((await call('/v1/lookup', {}, luna)).status, 401);
  assert.equal((await call('/v1/hello', { name: 'Luna', secret: secretA })).status, 403);
});

test('oversized and malformed bodies are rejected', async () => {
  const big = await handle(new Request('http://x/v1/hello', { method: 'POST', body: 'x'.repeat(20000) }), env, store, clock);
  assert.equal(big.status, 413);
  const bad = await handle(new Request('http://x/v1/hello', { method: 'POST', body: '{nope' }), env, store, clock);
  assert.equal(bad.status, 400);
});

test('health and unknown routes', async () => {
  const res = await handle(new Request('http://x/v1/health'), env, store, clock);
  assert.deepEqual(await res.json(), { ok: true, online: 0 });
  const get = await handle(new Request('http://x/v1/hello'), env, store, clock);
  assert.equal(get.status, 405);
  const token = await login('Luna');
  assert.equal((await call('/v1/nope', {}, token)).status, 404);
});
