package dev.net64.ghostship

import android.view.View
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat

/**
 * Keeps this view's content clear of the status bar, navigation bar and camera
 * cutout, on top of whatever padding it already has.
 *
 * From API 35 the system lays every window out edge to edge and ignores the old
 * opt-outs, so a screen that does nothing about it gets its content drawn
 * underneath the bars — which put the launcher's Choose ROM button behind the
 * navigation bar. The game does the opposite and hides the bars outright; see
 * [MainActivity].
 */
fun View.padForSystemBars() {
    val left = paddingLeft
    val top = paddingTop
    val right = paddingRight
    val bottom = paddingBottom

    ViewCompat.setOnApplyWindowInsetsListener(this) { view, insets ->
        val bars = insets.getInsets(
            WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout()
        )
        view.setPadding(left + bars.left, top + bars.top, right + bars.right, bottom + bars.bottom)
        insets
    }
}
