#pragma once
// フィールド（地形・木・岩・町・ボス部屋の見た目と当たり判定）
// Assets/Models/field_floorN/model.cfg があれば地形モデルとして描画し、無ければ手続き生成した地面を使う。
#include "Floor.h"
#include "DxLib.h"
#include <vector>
#include <string>

namespace ag {

struct Obstacle {
    VECTOR pos;
    float radius;
};

class Field {
public:
    ~Field();
    void Build(const FloorDef& def);
    void Release();
    void Draw(const VECTOR& cameraPos) const;
    void DrawLabels() const; // 3D 位置に 2D の看板テキスト

    void ResolveCollision(VECTOR& pos, float radius) const;
    bool InTown(const VECTOR& p) const;
    bool InBossArena(const VECTOR& p) const;
    const FloorDef& Def() const { return *def_; }
    float PathX(float z) const;

private:
    struct Prop {
        enum class Kind { Tree, Rock, House, Guild, Pillar, Fountain, Post } kind;
        VECTOR pos;
        float size;
        float yaw;
        unsigned int color;
    };
    void DrawProp(const Prop& p) const;

    const FloorDef* def_ = nullptr;
    std::vector<VERTEX3D> vertices_;
    std::vector<unsigned short> indices_;
    std::vector<Prop> props_;
    std::vector<Obstacle> obstacles_;
    int groundTexture_ = -1;
    int fieldModel_ = -1;
};

} // namespace ag
