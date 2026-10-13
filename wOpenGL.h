#pragma once

#include "cuicui_core/cuicui.h"

// UWP: you must install https://www.nuget.org/packages/ANGLE.WindowsStore/ through nugets

// Warning: using manual loop
//          -> on android: opengl is NOT in UI thread
//          -> on uwp and ios: opengl is in UI thread
// Warning: using manual loop
//          i don't know, but i think on uwp opengl will NOT be in UI thread

#ifdef CUICUI_UWP
    // Enable function definitions in the GL headers below
    #define GL_GLEXT_PROTOTYPES

// OpenGL ES includes
#include <GLES3/gl3.h>

    // EGL includes
    #include <EGL/egl.h>
    #include <EGL/eglext.h>
    #include <EGL/eglplatform.h>

    #include <angle_windowsstore.h>
#endif

#ifdef CUICUI_IOS
    #include <OpenGLES/ES3/gl.h>
    #import <GLKit/GLKit.h>
#endif

#ifdef CUICUI_UWP
namespace cui {
class OpenGLES
{
public:
	OpenGLES();
	~OpenGLES();

	EGLSurface CreateSurface(Windows::UI::Xaml::Controls::SwapChainPanel^ panel, const Windows::Foundation::Size* renderSurfaceSize, const float* renderResolutionScale);
	void GetSurfaceDimensions(const EGLSurface surface, EGLint *width, EGLint *height);
	void DestroySurface(const EGLSurface surface);
	void MakeCurrent(const EGLSurface surface);
	EGLBoolean SwapBuffers(const EGLSurface surface);
	void Reset();

private:
	void Initialize();
	void Cleanup();

private:
	EGLDisplay mEglDisplay;
	EGLContext mEglContext;
	EGLConfig  mEglConfig;
};
} // namespace cui
#include <concrt.h>
#endif



#ifdef CUICUI_IOS
namespace cui {
class wOpenGL;
// source : http://nehe.gamedev.net/tutorial/ios_lesson_01__setting_up_gl_es/44001/
// Note: we dont use GLKit because the result is not the same than on other platforms (viewport is not the same, ...)
} // namespace cui
@interface UICuiOpenGlView : UIView
{
    GLint framebufferWidth;
    GLint framebufferHeight;
    GLuint defaultFramebuffer, colorRenderbuffer, depthRenderbuffer;
}
@property (nonatomic, retain) EAGLContext *context;
-(void)setPtr : (cui::wOpenGL*)_ptr;
-(GLint)getFrameBufferWidth;
-(GLint)getFrameBufferHeight;
-(void)AllowRender;
@end

#endif

#ifdef CUICUI_WEB
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#ifdef CUICUI_ANDROID
// https://code.tutsplus.com/tutorials/how-to-use-opengl-es-in-android-apps--cms-28464

// OpenGL ES includes
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#endif

namespace cui {
class wOpenGL : public wWidget
{
protected:
#ifdef CUICUI_UWP
	SwapChainPanel ^swapChainPanel;
	OpenGLES *mOpenGLES;
	EGLSurface mRenderSurface;	
	Concurrency::critical_section mRenderSurfaceCriticalSection;
	EGLint frameBufferWidth = 0;
	EGLint frameBufferHeight = 0;
#endif


public: // android need access to these:
	std::thread::id glThreadId; // on Android the OpenGL thread is not the UI thread
	bool firstFrame = true;
    bool frameDrawn=false;
    
public: //protected (ios need it public)
	std::function<void()> renderLoopCallback = nullptr;
    std::function<void()> postRenderCallback = nullptr;
    
    
#ifdef CUICUI_IOS
public:
    UICuiOpenGlView *uiOpenGlView;
    int iosMaxFps;
#endif
#ifdef CUICUI_ANDROID
	EGLint frameBufferWidth = 0;
	EGLint frameBufferHeight = 0;
#endif
    
public:	
	wOpenGL();
    
    ~wOpenGL();
    
    void Cleanup();

    void OnNativeFrameChanged(double width, double height) override; // wWidget extension point
	
    void ResetFrameDrawnFlag() { frameDrawn=false; }

	// false on the platforms where the widget is an empty placeholder (web)
	static bool IsSupported()
	{
#if defined(CUICUI_ANDROID) || defined(CUICUI_IOS) || defined(CUICUI_UWP)
		return true;
#else
		return false;
#endif
	}
    
	void AssertOnOpenGlThread() // on Android the OpenGL thread is not the UI thread
	{
#ifdef CUICUI_DEBUG
		std::thread::id currentThreadId = std::this_thread::get_id();
		if (glThreadId != currentThreadId)
            cui_assert(false);
#endif
	}
	void AssertNotOnOpenGlThread()
	{
#ifdef CUICUI_DEBUG
		std::thread::id currentThreadId = std::this_thread::get_id();
		if (glThreadId == currentThreadId)
            cui_assert(false);
#endif
	}

	int GetFrameBufferWidth()
	{
#ifdef CUICUI_IOS
        return [uiOpenGlView getFrameBufferWidth];
#endif
#ifdef CUICUI_UWP
		return frameBufferWidth;
#endif
#ifdef CUICUI_ANDROID
		return frameBufferWidth;
#endif
		return -1;
	}

	int GetFrameBufferHeight()
	{
#ifdef CUICUI_IOS
        return [uiOpenGlView getFrameBufferHeight];
#endif
#ifdef CUICUI_UWP
		return frameBufferHeight;
#endif
#ifdef CUICUI_ANDROID
		return frameBufferHeight;
#endif
		return -1;
	}

	void SetRenderLoopCallback(std::function<void()> f) // WARNING: callback WON'T be on the UI thread
	{
#ifdef CUICUI_IOS
        cui_assert(false); // nyi not tested dunno
#endif
        cui_assert(renderLoopCallback == nullptr);
		renderLoopCallback = f;
		StartRenderLoop(); 
	}
	void SetManualRenderLoopCallback(std::function<void()> f)
	{
        cui_assert(renderLoopCallback == nullptr);
		renderLoopCallback = f;		
	}

	void StartRenderLoop();
	void ManualLoop();

public:
	static wOpenGL* Create(Length xem, Length yem)
	{
		wOpenGL *c = new wOpenGL();
		c->Xem(xem)->Yem(yem);
		return c;
	}
};

} // namespace cui