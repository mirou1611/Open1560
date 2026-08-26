package com.open1560.app;

import org.libsdl.app.SDLActivity;

/**
 * Open1560 on Android.
 *
 * The game reads its data (audio.ar, core.ar, ui.ar and friends) from this
 * app's external files directory, which the native side chdir()s into at
 * startup:
 *
 *     /sdcard/Android/data/com.open1560.app/files/
 */
public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }
}
