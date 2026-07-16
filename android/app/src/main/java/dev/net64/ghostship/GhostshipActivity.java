package dev.net64.ghostship;

import org.libsdl.app.SDLActivity;

public class GhostshipActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL2",
            "main"
        };
    }
}
