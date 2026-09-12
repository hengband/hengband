# 職業技能定義

`lib/edit/ClassSkillDefinitions.jsonc` は29職業の技能初期値と上限を定義します。
`classes` は `PlayerClassType` のID順（0から28）に全職業を記載します。

- `weapons`: `BOW` / `DIGGING` / `HAFTED` / `POLEARM` / `SWORD`。
  各 `start_ranks` / `max_ranks` は添字がsvalに対応する64要素の配列です。
  各要素のコメントにsval番号と旧定義の名前を記載しています。値を変更するときは対象のコメントを確認してください。
  存在しないアイテムのsvalも含め、既存の技能テーブル全体を保持します。
  0〜4は `UNSKILLED` / `BEGINNER` / `SKILLED` / `EXPERT` / `MASTER` を表し、
  Readerで経験値0 / 4000 / 6000 / 7000 / 8000に変換します。
- `skills`: `MARTIAL_ARTS` / `TWO_WEAPON` / `RIDING` / `SHIELD`。
  `start_exp` / `max_exp` はランクではなく、0〜8000の経験値です。
- 初期値は上限以下でなければなりません。IDの重複・欠落、未知のキー、型・範囲・配列長の不正を拒否します。

JSON Schemaは型・範囲・必須項目を検証し、Readerはそれに加えてID順序と初期値≦上限を検証します。
職業レコードは全項目の検証が成功してから技能テーブルへ反映します。

Readerはエラーコードに加えて職業ID・項目パス・原因を `error()` から返します。
画面への出力は初期化側で行い、日本語版・英語版それぞれの言語で停止理由を表示します。
項目パスと職業IDは共通です。英語版では例えば次の形式になります。

```text
ClassSkillDefinitions.jsonc: class 7 at $.weapons.SWORD.start_ranks[63]: start rank 4 exceeds maximum 3
```

診断情報は `read()` ごとにリセットされ、正常終了時は空になります。

旧形式との全件比較（`pyjson5` が必要）:

```sh
python3 tools/verify-class-skill-migration.py --legacy-ref a82db63253
```

この移行では職業IDと技能値は変更せず、セーブ形式も変更しません。
定義ファイルのハッシュは、従来のテキスト行からJSON正規化後の値へ計算対象が変わるため変化します。
