# 3Dモデルの差し替え

`Assets/Models/<キー>/model.cfg` を置くと、そのキーのキャラクターが外部モデルで描画されます。
フォルダや model.cfg が無い場合は、球・カプセルで作ったサンプルモデルが表示されます。

| キー | 対象 |
|---|---|
| `player` | プレイヤー |
| `wolf` | 草原ウルフ |
| `boar` | 岩角イノシシ |
| `goblin` | ゴブリン兵 |
| `boss_floor1` | 第1層ボス「灰牙の獣王 グラウゼル」 |
| `field_floor1` | 第1層の地形（任意。置くと手続き生成の地面の代わりに描画） |

## 対応形式

DxLib の `MV1LoadModel` が読める形式です: `.mv1` / `.x` / `.pmx` / `.pmd` / `.mqo` / `.fbx`
（itch.io で配布されている `.fbx` はそのまま使えることが多いです。`.glb/.gltf` は Blender 等で `.fbx` に変換してください）

## model.cfg の書式

```ini
# モデル本体（このフォルダからの相対パス）
file=Knight.fbx

# 見た目の調整
scale=1.0        # 身長 170 前後になるよう調整（1 ワールド単位 ≒ 1cm）
rot_y=180        # 正面が逆なら 0 にする（度）
y_offset=0       # 足が地面に埋まる/浮く場合に調整
anim_fps=30      # アニメ時間の単位（FBX は通常 30）

# アニメーション（5 種類 + 任意でスキル）
#   数字        … モデル内のアニメ番号
#   ファイル名   … 別ファイル（Mixamo のように1ファイル1アニメの場合）
#   ファイル:番号 … 別ファイル内の番号
idle=0
move=1
attack=2
skill=Attack_Spin.fbx
hit=Hit.fbx:0
death=4

# 再生速度の倍率（任意）
move_speed=1.2
```

- 攻撃・スキル・被弾・死亡のアニメは、ゲーム側のモーション時間に合わせて自動で速度調整されます。
- `skill` を省略すると `attack` が使われます。
- アニメ番号が分からない場合は DxLib 付属の「DxLibModelViewer」でモデルを開くと確認できます。
- ルートモーション（アニメ自体がキャラを前進させるもの）は、Mixamo なら「In Place」にチェックしてダウンロードしてください。

サンプル: [_sample/model.cfg.sample](_sample/model.cfg.sample)
