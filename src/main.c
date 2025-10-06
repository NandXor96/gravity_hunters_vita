#include "app/app.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

int main(int argc, char **argv) {

    (void)argc;
    (void)argv;
    if (!app_create())
        return 1;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;

}
        app_update();
    }
    app_destroy();
    return 0;
}
