package org.dolphinemu.dolphinemu.utils

import android.widget.CompoundButton
import androidx.core.view.HapticFeedbackConstantsCompat
import androidx.core.view.ViewCompat
import com.google.android.material.slider.Slider

/**
 * Wrapper class that enhances listeners with haptic feedback.
 */
object HapticListener {

    /**
     * Wraps a [Slider.OnChangeListener] with haptic feedback.
     *
     * @param listener The [Slider.OnChangeListener] to be wrapped. Can be null.
     * @return A new listener which wraps [listener] with haptic feedback.
     */
    fun wrapOnChangeListener(listener: Slider.OnChangeListener?): Slider.OnChangeListener {
        return Slider.OnChangeListener { slider: Slider, value: Float, fromUser: Boolean ->
            listener?.onValueChange(slider, value, fromUser)
            if (fromUser) {
                ViewCompat.performHapticFeedback(slider, HapticFeedbackConstantsCompat.CLOCK_TICK)
            }
        }
    }

    /**
     * Wraps a [CompoundButton.OnCheckedChangeListener] with haptic feedback.
     *
     * @param listener The [CompoundButton.OnCheckedChangeListener] to be wrapped. Can be null.
     * @return A new listener which wraps [listener] with haptic feedback.
     */
    fun wrapOnCheckedChangeListener(listener: CompoundButton.OnCheckedChangeListener?): CompoundButton.OnCheckedChangeListener {
        return CompoundButton.OnCheckedChangeListener { buttonView: CompoundButton, isChecked: Boolean ->
            listener?.onCheckedChanged(buttonView, isChecked)
            /** Using old constants because [HapticFeedbackConstantsCompat.TOGGLE_ON]
             *  and [HapticFeedbackConstantsCompat.TOGGLE_OFF] don't seem to work. */
            val feedbackConstant = if (buttonView.isChecked) {
                HapticFeedbackConstantsCompat.CONTEXT_CLICK
            } else HapticFeedbackConstantsCompat.CLOCK_TICK
            ViewCompat.performHapticFeedback(buttonView, feedbackConstant)
        }
    }
}
