# Mochi Online service

Small Cloudflare Worker that lets Mochi clients find each other: who runs Mochi on this server, and how their name, tag and cosmetics look. No Minecraft traffic goes through it. The concept is in `docs/ONLINE.md`.

## Run it locally

```
cd server
npm run dev        # http://127.0.0.1:8787, data lives in memory
npm test           # runs the tests against the memory store and against the real SQL (node:sqlite)
```

Point the client at it: Mochi Online, "Service address" = `http://127.0.0.1:8787`.

## Publish it

You need a Cloudflare account (the free plan is enough to start) and `wrangler`.

```
cd server
npx wrangler d1 create mochi-online
npx wrangler kv namespace create LIMITS
```

Put the two ids into `wrangler.toml`, then:

```
npx wrangler d1 execute mochi-online --remote --file schema.sql
npx wrangler secret put ADMIN_KEY
npx wrangler deploy
```

Wrangler prints the address. Enter it in the client as the service address (or make it the default in `dll/src/modules/online/MochiOnline.hpp`).

## Calls

All bodies are JSON, all answers are JSON. Everything except `hello` and `health` needs `Authorization: Bearer <token>` from `hello`.

| Call | What it does |
|---|---|
| `POST /v1/hello` | `{name, secret, client, visible, style, worn}` signs in and returns `{token, ttl, style, worn, filtered}`. The `secret` is a random 48 character hex string the client creates once and keeps. The first install that uses a gamertag owns it, others get 403 until it has been silent for 30 days. |
| `POST /v1/profile` | `{visible, style, worn}` saves the look. Tags that fail the filter are blanked and `filtered` is true. |
| `POST /v1/presence` | `{server}` heartbeat, every 60 seconds. |
| `POST /v1/lookup` | `{names: [...]}` (at most 100) returns `{users: [{name, style, worn}], online}` for visible users seen in the last 150 seconds. |
| `POST /v1/bye` | ends the session and hides the user. |
| `POST /v1/forget` | deletes everything stored about the gamertag. |
| `POST /v1/admin/block` | `{name, reason}` with header `X-Admin-Key`, blocks a gamertag. |
| `GET /v1/health` | `{ok, online}` |

`style` is `{mode: solid|gradient|rainbow|pulse, a, b, speed, tag, tagColor, heart}` with colors as `#rrggbb`. `worn` is a list of `{id, tint: [#rrggbb, ...]}` with at most 8 items.

## What it stores

Gamertag, a hash of the install secret, style, worn cosmetics, server name, client version, last seen. Nothing from chat, no worlds. IP addresses only live in the rate limit counters, which expire after two minutes. Rows nobody has touched for 90 days are deleted by a daily job.

## Limits

Ten `hello` per minute per address, 30 calls per route per minute per gamertag, bodies up to 16 KB.

## Not done yet

- Proof that you own the gamertag (the Xbox sign-in token). Until then the first claim wins, and the 30 day rule is the only way back for someone who lost their secret.
- Reports and a review tool for tags. Right now there is a word filter and the admin block.
