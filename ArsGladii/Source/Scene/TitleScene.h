#pragma once
#include "Scene.h"
#include "../Save/SaveData.h"

namespace ag {

class TitleScene : public Scene {
public:
    void Enter() override;
    void Update(float dt) override;
    void Draw() override;

private:
    float time_ = 0.0f;
    int cursor_ = 0;
    SaveHeader save_;
};

} // namespace ag
