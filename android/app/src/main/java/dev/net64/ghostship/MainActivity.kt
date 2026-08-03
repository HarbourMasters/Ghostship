package dev.net64.ghostship

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.Button
import android.widget.FrameLayout
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import org.libsdl.app.SDLActivity

/**
 * The game.
 *
 * The on-screen controller is the engine's own (src/port/ui/TouchControls.cpp),
 * drawn inside the GL surface — nothing here draws controls. The only thing
 * layered over SDL is a Mods button, and it only appears while the engine's
 * menu is up, because that is where settings-shaped actions belong.
 *
 * [LauncherActivity] guarantees sm64.o2r exists before this activity starts.
 */
class MainActivity : SDLActivity() {

    private lateinit var modsButton: Button

    private var menuOpen = false

    private val handler = Handler(Looper.getMainLooper())
    private val menuWatcher = object : Runnable {
        override fun run() {
            syncMenuState()
            handler.postDelayed(this, MENU_POLL_MS)
        }
    }

    // org/libsdl/app is kept byte-identical to the SDL release libultraship
    // pins, so the game library is named here rather than patched in there.
    // SDLActivity refuses to start if the Java glue and libSDL2.so disagree on
    // their version, so both have to move together.
    override fun getLibraries(): Array<String> = arrayOf("SDL2", "main")

    private external fun isMenuOpen(): Boolean

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        goFullscreen()
        addModsButton()
    }

    /**
     * Edge to edge, with the status and navigation bars hidden.
     *
     * From API 35 the system no longer honours the old fullscreen window flags,
     * so the bars have to be dismissed through the insets controller instead —
     * without this the navigation bar sits on top of the game. Transient-by-swipe
     * means a stray swipe shows them briefly rather than resizing the window and
     * forcing the engine to rebuild its framebuffers.
     */
    private fun goFullscreen() {
        WindowCompat.setDecorFitsSystemWindows(window, false)

        WindowInsetsControllerCompat(window, window.decorView).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        // Render into the camera cutout as well, so the game gets the whole
        // panel instead of being letterboxed beside it.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            window.attributes.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        // The bars come back whenever focus is lost — to the mods manager, a
        // notification shade pull, anything. Put them away again on the way in.
        if (hasFocus) {
            goFullscreen()
        }
    }

    override fun onResume() {
        super.onResume()
        // Re-sync immediately as well as on the timer, so coming back from the
        // mods manager never leaves the button showing a stale menu state.
        syncMenuState()
        handler.removeCallbacks(menuWatcher)
        handler.postDelayed(menuWatcher, MENU_POLL_MS)
    }

    override fun onPause() {
        handler.removeCallbacks(menuWatcher)
        super.onPause()
    }

    override fun onDestroy() {
        handler.removeCallbacks(menuWatcher)
        super.onDestroy()
    }

    /**
     * The engine's menu can be opened by its own on-screen button, a keyboard,
     * or a gamepad, and it can close itself. Asking the engine what it is doing
     * is what keeps this button from drifting out of sync with it.
     */
    private fun syncMenuState() {
        val open = runCatching { isMenuOpen() }.getOrDefault(false)
        if (open == menuOpen) return

        menuOpen = open
        modsButton.visibility = if (open) android.view.View.VISIBLE else android.view.View.GONE
    }

    /**
     * Bottom-left, which is clear of the engine's own menu toggle in the top-left
     * corner and of the menu itself.
     */
    private fun addModsButton() {
        val margin = (16 * resources.displayMetrics.density).toInt()

        modsButton = Button(this).apply {
            text = getString(R.string.mods_button)
            textSize = 12f
            isAllCaps = false
            visibility = android.view.View.GONE
            setOnClickListener { startActivity(Intent(context, ModsActivity::class.java)) }
        }

        val layout = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            Gravity.BOTTOM or Gravity.START
        ).apply { setMargins(margin, margin, margin, margin) }

        findViewById<ViewGroup>(android.R.id.content).addView(modsButton, layout)
    }

    private companion object {
        /** Fast enough to feel immediate on a menu toggle, cheap enough to ignore. */
        const val MENU_POLL_MS = 150L
    }
}
