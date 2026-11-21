#pragma once
// シーン基底クラス（タイトル / フィールド。将来: 町内部・ダンジョン・ギルド画面など）

namespace ag {

enum class SceneId { None, Title, Field, Quit };

struct SceneRequest {
    SceneId next = SceneId::None;
    bool continueGame = false; // Field へ遷移時: セーブデータから再開
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual void Enter() {}
    virtual void Exit() {}
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;

    SceneRequest TakeRequest()
    {
        SceneRequest r = request_;
        request_ = SceneRequest{};
        return r;
    }

protected:
    void RequestScene(SceneId id, bool continueGame = false)
    {
        request_.next = id;
        request_.continueGame = continueGame;
    }

private:
    SceneRequest request_;
};

} // namespace ag
