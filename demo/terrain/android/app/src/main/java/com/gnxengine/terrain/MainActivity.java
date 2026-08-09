package com.gnxengine.terrain;

import org.libsdl.app.SDLActivity;

/**
 * GNXEngine Terrain Demo 的 Android 入口。
 *
 * 继承 SDLActivity，指定 native 库为 "terrain"（libterrain.so）。
 * SDL2 以静态库形式链接进 libterrain.so（SDL2-static），
 * 因此运行时只需加载 terrain 一个共享库，SDL 会调用其中的 SDL_main() 入口。
 */
public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "terrain"
        };
    }
}
