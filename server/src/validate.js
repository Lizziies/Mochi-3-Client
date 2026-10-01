const modes = ['solid', 'gradient', 'rainbow', 'pulse'];

export const nameOk = (name) => typeof name === 'string' && /^[A-Za-z0-9 _.-]{1,32}$/.test(name) && name.trim() === name;

export const keyOf = (name) => name.toLowerCase();

const hex = (value, fallback) => (typeof value === 'string' && /^#[0-9a-fA-F]{6}$/.test(value) ? value.toLowerCase() : fallback);

export function cleanTag(raw) {
  if (typeof raw !== 'string') return '';
  let out = '';
  let points = 0;
  for (const ch of raw) {
    const code = ch.codePointAt(0);
    if (code < 0x20 || code === 0x7f || ch === '§') continue;
    if (points >= 16) break;
    out += ch;
    points++;
  }
  return out.trim();
}

export function cleanStyle(raw, filter) {
  const s = raw && typeof raw === 'object' ? raw : {};
  const mode = modes.includes(s.mode) ? s.mode : 'solid';
  const speed = Math.min(5, Math.max(0.1, Number.isFinite(s.speed) ? s.speed : 1));
  let tag = cleanTag(s.tag);
  const filtered = tag !== '' && !filter(tag);
  if (filtered) tag = '';
  return {
    style: {
      mode,
      a: hex(s.a, '#ff7eb6'),
      b: hex(s.b, '#ffffff'),
      speed: Math.round(speed * 100) / 100,
      tag,
      tagColor: hex(s.tagColor, '#ff7eb6'),
      heart: s.heart !== false,
    },
    filtered,
  };
}

export function cleanWorn(raw) {
  if (!Array.isArray(raw)) return [];
  const out = [];
  for (const item of raw) {
    if (out.length >= 8) break;
    if (!item || typeof item.id !== 'string' || !/^[a-z0-9_]{1,40}$/.test(item.id)) continue;
    const tint = Array.isArray(item.tint) ? item.tint.slice(0, 4).map((c) => hex(c, '#ffffff')) : [];
    out.push({ id: item.id, tint });
  }
  return out;
}

export function cleanServer(raw) {
  if (typeof raw !== 'string') return '';
  return [...raw].filter((c) => c.codePointAt(0) >= 0x20).slice(0, 48).join('').trim();
}
