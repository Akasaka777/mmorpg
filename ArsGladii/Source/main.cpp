// Ars Gladii - エントリーポイント
#include "DxLib.h"
#include "Core/Game.h"

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    ag::Game game;
    if (!game.Init()) {
        game.Shutdown();
        return -1;
    }
    game.Run();
    game.Shutdown();
    return 0;
}
