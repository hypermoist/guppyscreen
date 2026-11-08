package io.github.pellcorp.grumpyscreen;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.File;

/** The full-screen surface grumpyscreen renders into. */
@SuppressLint("ViewConstructor")
public class GrumpySurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private final File configFile;
    private final File moonrakerFile;
    private final File overrideFile;

    public GrumpySurfaceView(Context context, File configFile, File moonrakerFile, File overrideFile) {
        super(context);
        this.configFile = configFile;
        this.moonrakerFile = moonrakerFile;
        this.overrideFile = overrideFile;
        getHolder().addCallback(this);
        setKeepScreenOn(true);  // grumpyscreen's own screen timeout decides
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (getDisplay() != null) GrumpyNative.setRefreshRate(getDisplay().getRefreshRate());
        GrumpyNative.surfaceChanged(holder.getSurface(), width, height,
                configFile.getAbsolutePath(), moonrakerFile.getAbsolutePath(),
                overrideFile.getAbsolutePath());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        GrumpyNative.surfaceDestroyed();
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        boolean down = action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_CANCEL;
        // LVGL here is single touch: follow the first finger
        GrumpyNative.touch((int) event.getX(0), (int) event.getY(0), down);
        return true;
    }
}
