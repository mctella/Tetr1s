#include "Engine.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "Tetris.h"
#include "MainMenu.h"

using namespace TetrisConst;

int main(int argc, char* argv[]){
    char title[] = "Tetris";
    Engine engine(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO, SCREEN_WIDTH, SCREEN_HEIGHT, title);
    engine.setFPS(240.0);
    engine.setCursor(false);

    engine.getContext().scenes.changeScene(std::make_unique<MainMenu>());
    engine.run();
    return 0;
}
