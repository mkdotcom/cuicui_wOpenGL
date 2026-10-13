package io.github.mkdotcom.cuicui.wopengl;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.widget.FrameLayout;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import io.github.mkdotcom.cuicui.CuicuiActivity;

// Android side of the cuicui_wOpenGL module.
// Called from C++ (wOpenGL.cpp) through JNI; calls back into C++ through the native methods below.
// It only uses the generic extension points of the core activity (cuiGetElementView / cuiGetElementWidgetPtr).
public class CuicuiWOpenGL
{
    // Creates a GLSurfaceView (OpenGL ES 2) inside the native container view of the wOpenGL widget.
    public static int createAndAddToElement(CuicuiActivity activity, int elementParentViewId)
    {
        // the core's container view (a CuicuiActivity.CustomFrameLayout, which holds the C++ widget pointer)
        final CuicuiActivity.CustomFrameLayout parent = (CuicuiActivity.CustomFrameLayout) activity.cuiGetElementView(elementParentViewId);

        final GLSurfaceView sv = new GLSurfaceView(activity);
        final int id = activity.cuiNewViewId();
        sv.setId(id);
        sv.setEGLContextClientVersion(2); // OpenGL ES 2
        parent.addView(sv);

        sv.setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
                sv.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY); // render only when requested (ManualLoop); for automatic: RENDERMODE_CONTINUOUSLY
                Log.d("cuicui", "wOpenGL onSurfaceCreated");
            }
            @Override
            public void onSurfaceChanged(GL10 gl10, int width, int height) {
                GLES20.glViewport(0, 0, width, height);
                // widgetPtr is read here (not at creation): it may be set after the native view is created
                CuicuiWOpenGL.onSurfaceChanged(parent.widgetPtr, width, height);
            }
            @Override
            public void onDrawFrame(GL10 gl10) {
                CuicuiWOpenGL.onDrawFrame(parent.widgetPtr);
            }
        });

        return id;
    }

    public static void requestRenderFrame(CuicuiActivity activity, int elementParentViewId)
    {
        FrameLayout parent = activity.cuiGetElementView(elementParentViewId);
        if (parent == null || parent.getChildCount() == 0) return;
        GLSurfaceView sv = (GLSurfaceView) parent.getChildAt(0);
        sv.requestRender();
    }

    // implemented in wOpenGL.cpp (called on the GL thread)
    static native void onDrawFrame(long widgetPtr);
    static native void onSurfaceChanged(long widgetPtr, int width, int height);
}
