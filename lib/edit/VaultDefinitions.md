# Vault definitions

`VaultDefinitions.jsonc` contains an ordered `vaults` array. Comments are allowed.
Each record has the following required fields; unknown fields are rejected.

| Field | Meaning |
| --- | --- |
| `id` | Unique, increasing ID, 0–32767. Gaps are allowed. |
| `name` | Nonempty vault name (shared by Japanese and English builds). |
| `type` | Room type: 7 (lesser), 8 (greater), or 17. |
| `rating` | Nonnegative rating, preserved from the old definition. |
| `height`, `width` | Dimensions in cells, 1–66 and 1–198 respectively. |
| `layout` | Exactly `height` strings, each exactly `width` printable ASCII bytes. |

Leading and trailing spaces in each layout string are significant. Do not trim
them. Escape quotes and backslashes using JSON syntax. The reader concatenates
rows without newlines. It checks dimensions, IDs, types and fields before storing
the record; errors identify the vault ID and JSON field path.
The root must be an object containing a nonempty `vaults` array. The CI validation
tool also checks ID ordering/uniqueness and layout dimensions after JSON Schema
validation, so these cross-field errors are rejected before running the game.

Map symbols retain their existing meanings in `src/room/rooms-vault.cpp`: `%`
marks the outer wall, `#` granite, `$` glass, `X` permanent rock, `Y` permanent
glass, `+` secret doors, `-` secret glass doors, `'` curtains, `^` traps, `*`
treasure or traps, and `&`, `@`, `9`, `8`, `,` monster/treasure placements.
Terrain symbols and unmarked floor cells are unchanged by this migration.

## Migration verification

Run `python tools/verify-vault-migration.py --legacy-ref <pre-migration-commit>`
with `pyjson5` installed. This compares every ID, name, type, rating, dimension
and concatenated layout byte, in order. The optional `--write` regenerates the
JSONC mechanically from that legacy revision and formats it using the repository's
Prettier configuration before replacing the file. This option additionally needs
Node.js and Prettier (on PATH, or available to `npx --no-install prettier`);
for example, install it with `npm install --global prettier`. Output uses LF on
every platform. If formatting fails, the existing file is left untouched.
Prettier may put short layout arrays on a single line; the strings still represent
individual map rows and no leading/trailing spaces inside them are removed.

Legacy ID 114 (`Lesser Vault (Interlock)`) declared height 17 and width 12, but
visually used 12 rows of 17 characters. The old reader concatenated these rows
and the game consumed them at width 12. The JSONC uses 17 rows of 12 characters
without changing the concatenated bytes or dimensions. Correcting the visual
design would change gameplay and is deliberately left to a separate change.

The definition hash changes with the serialization format; this does not change
vault IDs or the in-memory definitions. No legacy text fallback is provided.
