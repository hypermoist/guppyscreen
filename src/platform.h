#ifndef __GUPPY_PLATFORM_H__
#define __GUPPY_PLATFORM_H__

// libhv's hplatform.h defines OS_ANDROID as well; keep the two compatible.
#if (defined(ANDROID) || defined(__ANDROID__)) && !defined(OS_ANDROID)
  #define OS_ANDROID
#endif

#ifdef OS_ANDROID
// Opens the app's printer list (android/app/src/main/cpp/grumpy_android.cpp).
void android_show_printer_picker();
// Restarts the app: the Android "Restart GUI".
void android_restart_app();
// Turns the backlight off (true) or back on, for the screen timeout.
void android_set_display_sleep(bool sleeping);
// The nav bar glyphs rasterised at px (android/app/src/main/cpp/grumpy_icons.c),
// or NULL when materialdesign_font_40 is big enough.
extern "C" const struct _lv_font_t *android_icon_font(int px);
#endif

#endif // __GUPPY_PLATFORM_H__
