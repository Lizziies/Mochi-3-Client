const banned = [
  'nazi', 'hitler', 'heil', 'nigg', 'neger', 'fag', 'retard', 'spast', 'kike', 'chink', 'tranny',
  'fuck', 'shit', 'bitch', 'cunt', 'whore', 'slut', 'rape', 'kys', 'pedo',
  'arsch', 'hure', 'fotze', 'wichser', 'hurensohn', 'scheiss', 'missgeburt',
];

const reserved = ['admin', 'moderator', 'staff', 'owner', 'support'];

const leet = { 0: 'o', 1: 'i', 3: 'e', 4: 'a', 5: 's', 7: 't', 8: 'b', '@': 'a', $: 's', '!': 'i' };

export function normalize(text) {
  let out = '';
  for (const ch of text.toLowerCase()) out += leet[ch] ?? ch;
  return out.replace(/ß/g, 'ss').replace(/[^a-z]/g, '');
}

export function tagAllowed(tag) {
  const flat = normalize(tag);
  if (!flat) return true;
  return ![...banned, ...reserved].some((word) => flat.includes(word));
}
