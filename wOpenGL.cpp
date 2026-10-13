#include "wOpenGL.h"
namespace cui {} using namespace cui; // for code outside the namespace (ObjC blocks, main)


#include <iostream>
#include <chrono>


namespace cui {

#ifdef CUICUI_UWP
using namespace Platform;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

OpenGLES::OpenGLES() :
	mEglConfig(nullptr),
	mEglDisplay(EGL_NO_DISPLAY),
	mEglContext(EGL_NO_CONTEXT)
{
	Initialize();
}

OpenGLES::~OpenGLES()
{
	Cleanup();
}
#endif

#ifdef CUICUI_IOS
} // namespace cui
@interface UICuiOpenGlView()
- (void)createFramebuffer;
- (void)deleteFramebuffer;
- (void)setupGL;
- (void)uninit;
@end

@implementation UICuiOpenGlView {
    wOpenGL *wOpenGLPtr;
    bool allowRender;
    CADisplayLink *displayLink;
    int createdFrameBuffers;
    int createdRenderBuffers;
}
@synthesize context;
-(void)AllowRender
{
    allowRender=true;
}
-(void)uninit
{
    [displayLink invalidate];
    displayLink = nullptr;
}
-(void)setPtr:(wOpenGL*)_ptr {
    wOpenGLPtr = _ptr;
}
+ (Class)layerClass
{
    return [CAEAGLLayer class];
}
- (void)setupGL
{
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;

    eaglLayer.opaque = TRUE;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                    nil];
    
    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!self.context) {
        cui_assert(false);
        NSLog(@"Failed to create ES context");
    }
    
    defaultFramebuffer = 0;
    colorRenderbuffer = 0;
    depthRenderbuffer = 0;
    
    createdFrameBuffers = 0;
    createdRenderBuffers = 0;

    displayLink = [CADisplayLink displayLinkWithTarget : self selector : @selector(timerFired : )];
    
    // these two lines enable "high fps"; there is also a CADisableMinimumFrameDuration to true in the info.plist
    NSInteger maxFPS = [UIScreen mainScreen].maximumFramesPerSecond;
    displayLink.preferredFramesPerSecond = maxFPS; // apparently we can set 0 which means "maximum"
    wOpenGLPtr->iosMaxFps = (int)maxFPS;
    
    [displayLink addToRunLoop : [NSRunLoop currentRunLoop] forMode : NSDefaultRunLoopMode];
    
    allowRender=true;
}
- (void) timerFired:(CADisplayLink *)sender {
    
    if (wOpenGLPtr->firstFrame == true)
    {
        wOpenGLPtr->firstFrame = false;
        wOpenGLPtr->glThreadId = std::this_thread::get_id();
    }
    
    if (allowRender==false) return;

    //double duration = [sender duration]; always returns 0,16 (if 60fps screen)
    
    allowRender=false;
    
    [self drawFrame];
    
    wOpenGLPtr->frameDrawn = true;
}
-(GLint)getFrameBufferWidth
{
    return framebufferWidth;
}
-(GLint)getFrameBufferHeight
{
    return framebufferHeight;
}
- (void)createFramebuffer
{
    //this method assumes, that the context is valid and current, and that the default framebuffer has not been created yet!
    //this works, because as soon as we call glGenFramebuffers the value will be > 0
    cui_assert(defaultFramebuffer == 0);
    
    NSLog(@"EAGLView: creating Framebuffer");
    
    // Create default framebuffer object and bind it
    glGenFramebuffers(1, &defaultFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
    createdFrameBuffers++;
    
    // Create color render buffer
    glGenRenderbuffers(1, &colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    createdRenderBuffers++;
    
    //get the storage from iOS so it can be displayed in the view
    [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    
    //get the frame's width and height
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
    
    //attach this color buffer to our framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);

    //check that our configuration of the framebuffer is valid
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
}

- (void)deleteFramebuffer
{
    NSLog(@"EAGLView: deleting Framebuffer");
    
    
    if (context)
    {
        
        
        [EAGLContext setCurrentContext:context];
        
        //if the default framebuffer has been set, delete it.
        if (defaultFramebuffer)
        {
            glDeleteFramebuffers(1, &defaultFramebuffer);
            defaultFramebuffer = 0;
            
            createdFrameBuffers--;
        }
        
        //same for the renderbuffers, if they are set, delete them
        if (colorRenderbuffer)
        {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
            colorRenderbuffer = 0;

            createdRenderBuffers--;
        }

    }
}

    //As soon as the view is resized or new subviews are added, this method is called,
    //apparently the framebuffers are invalid in this case so we delete them
    //and have them recreated the next time we draw to them
- (void)layoutSubviews
    {
        [self deleteFramebuffer];
    }
    
    //cleanup our view
- (void)dealloc
    {
        [self deleteFramebuffer];
    }
    
- (void)drawFrame
{
    //we need a context for rendering
    if (context != nil)
    {
        //make it the current context for rendering
        [EAGLContext setCurrentContext:context];
        
        //if our framebuffers have not been created yet, do that now!
        if (!defaultFramebuffer)
        {
            [self createFramebuffer];
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
        
        // open gl calls
        wOpenGLPtr->renderLoopCallback();
       
        //finally, get the color buffer we rendered to, and pass it to iOS
        //so it can display our awesome results!
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        [context presentRenderbuffer:GL_RENDERBUFFER];
        
        if (wOpenGLPtr->postRenderCallback != nullptr) wOpenGLPtr->postRenderCallback();
    }
    else
        NSLog(@"Context not set!");
}
@end
namespace cui {
#endif

void wOpenGL::Cleanup()
{
#ifdef CUICUI_IOS
    if (uiOpenGlView==nullptr) return;
    [uiOpenGlView uninit];
    uiOpenGlView = nullptr;
#endif
}

wOpenGL::~wOpenGL()
{
    Cleanup();
}

// called by the core (wWidget::SetPos_HW) once the native view has its new position and size
void wOpenGL::OnNativeFrameChanged(double width, double height)
{
#ifdef CUICUI_IOS
    [uiOpenGlView setFrame: CGRectMake(0, 0, width, height)];
    RunOnUiThread([this]()
    {
        if (CheckDefWidgetAlive(this))
            ManualLoop(); // we need to refresh
    });
#endif
}

#ifdef CUICUI_ANDROID
// The Java side lives in this module: android/io/github/mkdotcom/cuicui/wopengl/CuicuiWOpenGL.java
static const char* wOpenGL_JavaClass = "io/github/mkdotcom/cuicui/wopengl/CuicuiWOpenGL";

static jmethodID wOpenGL_GetStaticMethod(JNIEnv* env, jclass c, const char* name, const char* sig)
{
    jmethodID m = env->GetStaticMethodID(c, name, sig);
    cui_assert(m != nullptr);
    return m;
}
#endif



wOpenGL::wOpenGL() : wWidget()
{
    coreType = eCoreType::Custom;
    
#ifdef CUICUI_WEB
	// TODO: WebGL implementation (a <canvas> in the widget + emscripten_webgl_create_context)
	LogWarning("wOpenGL is not implemented on the web yet (empty widget)");
#endif

#ifdef CUICUI_UWP

	Native();

	swapChainPanel = ref new SwapChainPanel();	
	((Grid^)b)->Children->Append(swapChainPanel);

	// https://developernote.com/2015/11/combining-opengl-and-xaml-together-in-a-windows-store-app-uwp/

		// The app can configure the the SwapChainPanel which may boost performance.
		// By default, this template uses the default configuration.
	mOpenGLES = new OpenGLES();

	// with this the surface get the size of the wOpenGL (even if the wOpenGL widget is resized by user)
	mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, nullptr, nullptr);

	// You can configure the SwapChainPanel to render at a lower resolution and be scaled up to
	// the swapchain panel size. This scaling is often free on mobile hardware.
	//
	// One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
	//Size customRenderSurfaceSize = Size(800, 800);
	//mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, &customRenderSurfaceSize, nullptr);
	//
	// Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
	// e.g. if the SwapChainPanel is 1920x1280 then setting a factor of 0.5f will make the app render at 960x640
	// float customResolutionScale = 0.5f;
	// mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, nullptr, &customResolutionScale);
	// 

#endif
#ifdef CUICUI_IOS
    uiOpenGlView = [[UICuiOpenGlView alloc] initWithFrame:CGRectZero];
	uiView = (CuiUIView*)uiOpenGlView;
    [uiOpenGlView setPtr:this];
    [uiOpenGlView setupGL];
#endif
#ifdef CUICUI_ANDROID
    {
        JNIEnv* env = SafeJniEnv(globalCuiJniEnv);
        jclass c = jniCallManager.FindClass(env, wOpenGL_JavaClass);
        jmethodID m = wOpenGL_GetStaticMethod(env, c, "createAndAddToElement", "(Lio/github/mkdotcom/cuicui/CuicuiActivity;I)I");
        env->CallStaticIntMethod(c, m, globalCuiJniObj, (jint)theId);
    }
#endif
}

#ifdef CUICUI_UWP
void OpenGLES::Initialize()
{
	const EGLint configAttributes[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	const EGLint contextAttributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};

	const EGLint defaultDisplayAttributes[] =
	{
		// These are the default display attributes, used to request ANGLE's D3D11 renderer.
		// eglInitialize will only succeed with these attributes if the hardware supports D3D11 Feature Level 10_0+.
		EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

		// EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER is an optimization that can have large performance benefits on mobile devices.
		// Its syntax is subject to change, though. Please update your Visual Studio templates if you experience compilation issues with it.
		EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,

		// EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that enables ANGLE to automatically call 
		// the IDXGIDevice3::Trim method on behalf of the application when it gets suspended. 
		// Calling IDXGIDevice3::Trim when an application is suspended is a Windows Store application certification requirement.
		EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
		EGL_NONE,
	};

	const EGLint fl9_3DisplayAttributes[] =
	{
		// These can be used to request ANGLE's D3D11 renderer, with D3D11 Feature Level 9_3.
		// These attributes are used if the call to eglInitialize fails with the default display attributes.
		EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
		EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 9,
		EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 3,
		EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
		EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
		EGL_NONE,
	};

	const EGLint warpDisplayAttributes[] =
	{
		// These attributes can be used to request D3D11 WARP.
		// They are used if eglInitialize fails with both the default display attributes and the 9_3 display attributes.
		EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
		EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE,
		EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
		EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
		EGL_NONE,
	};

	EGLConfig config = NULL;

	// eglGetPlatformDisplayEXT is an alternative to eglGetDisplay. It allows us to pass in display attributes, used to configure D3D11.
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
	if (!eglGetPlatformDisplayEXT)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
	}

	//
	// To initialize the display, we make three sets of calls to eglGetPlatformDisplayEXT and eglInitialize, with varying 
	// parameters passed to eglGetPlatformDisplayEXT:
	// 1) The first calls uses "defaultDisplayAttributes" as a parameter. This corresponds to D3D11 Feature Level 10_0+.
	// 2) If eglInitialize fails for step 1 (e.g. because 10_0+ isn't supported by the default GPU), then we try again 
	//    using "fl9_3DisplayAttributes". This corresponds to D3D11 Feature Level 9_3.
	// 3) If eglInitialize fails for step 2 (e.g. because 9_3+ isn't supported by the default GPU), then we try again 
	//    using "warpDisplayAttributes".  This corresponds to D3D11 Feature Level 11_0 on WARP, a D3D11 software rasterizer.
	//

	// This tries to initialize EGL to D3D11 Feature Level 10_0+. See above comment for details.
	mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
	if (mEglDisplay == EGL_NO_DISPLAY)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
	}

	if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
	{
		// This tries to initialize EGL to D3D11 Feature Level 9_3, if 10_0+ is unavailable (e.g. on some mobile devices).
		mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
		if (mEglDisplay == EGL_NO_DISPLAY)
		{
			throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
		}

		if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
		{
			// This initializes EGL to D3D11 Feature Level 11_0 on WARP, if 9_3+ is unavailable on the default GPU.
			mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
			if (mEglDisplay == EGL_NO_DISPLAY)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
			}

			if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
			{
				// If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred.
				throw Exception::CreateException(E_FAIL, L"Failed to initialize EGL");
			}
		}
	}

	EGLint numConfigs = 0;
	if ((eglChooseConfig(mEglDisplay, configAttributes, &mEglConfig, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
	{
		throw Exception::CreateException(E_FAIL, L"Failed to choose first EGLConfig");
	}

	mEglContext = eglCreateContext(mEglDisplay, mEglConfig, EGL_NO_CONTEXT, contextAttributes);
	if (mEglContext == EGL_NO_CONTEXT)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to create EGL context");
	}
}

void OpenGLES::Cleanup()
{
	if (mEglDisplay != EGL_NO_DISPLAY && mEglContext != EGL_NO_CONTEXT)
	{
		eglDestroyContext(mEglDisplay, mEglContext);
		mEglContext = EGL_NO_CONTEXT;
	}

	if (mEglDisplay != EGL_NO_DISPLAY)
	{
		eglTerminate(mEglDisplay);
		mEglDisplay = EGL_NO_DISPLAY;
	}
}

void OpenGLES::Reset()
{
	Cleanup();
	Initialize();
}

EGLSurface OpenGLES::CreateSurface(SwapChainPanel^ panel, const Size* renderSurfaceSize, const float* resolutionScale)
{
	if (!panel)
	{
		throw Exception::CreateException(E_INVALIDARG, L"SwapChainPanel parameter is invalid");
	}

	if (renderSurfaceSize != nullptr && resolutionScale != nullptr)
	{
		throw Exception::CreateException(E_INVALIDARG, L"A size and a scale can't both be specified");
	}

	EGLSurface surface = EGL_NO_SURFACE;

	const EGLint surfaceAttributes[] =
	{
		// EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER is part of the same optimization as EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER (see above).
		// If you have compilation issues with it then please update your Visual Studio templates.
		EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
		EGL_NONE
	};

	// Create a PropertySet and initialize with the EGLNativeWindowType.
	PropertySet^ surfaceCreationProperties = ref new PropertySet();
	surfaceCreationProperties->Insert(ref new String(EGLNativeWindowTypeProperty), panel);

	// If a render surface size is specified, add it to the surface creation properties
	if (renderSurfaceSize != nullptr)
	{
		surfaceCreationProperties->Insert(ref new String(EGLRenderSurfaceSizeProperty), PropertyValue::CreateSize(*renderSurfaceSize));
	}

	// If a resolution scale is specified, add it to the surface creation properties
	if (resolutionScale != nullptr)
	{
		surfaceCreationProperties->Insert(ref new String(EGLRenderResolutionScaleProperty), PropertyValue::CreateSingle(*resolutionScale));
	}

	surface = eglCreateWindowSurface(mEglDisplay, mEglConfig, reinterpret_cast<IInspectable*>(surfaceCreationProperties), surfaceAttributes);
	if (surface == EGL_NO_SURFACE)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to create EGL surface");
	}

	return surface;
}

void OpenGLES::GetSurfaceDimensions(const EGLSurface surface, EGLint* width, EGLint* height)
{
	eglQuerySurface(mEglDisplay, surface, EGL_WIDTH, width);
	eglQuerySurface(mEglDisplay, surface, EGL_HEIGHT, height);
}

void OpenGLES::DestroySurface(const EGLSurface surface)
{
	if (mEglDisplay != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE)
	{
		eglDestroySurface(mEglDisplay, surface);
	}
}

void OpenGLES::MakeCurrent(const EGLSurface surface)
{
	if (eglMakeCurrent(mEglDisplay, surface, surface, mEglContext) == EGL_FALSE)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to make EGLSurface current");
	}
}

EGLBoolean OpenGLES::SwapBuffers(const EGLSurface surface)
{
	return (eglSwapBuffers(mEglDisplay, surface));
}
#endif



void wOpenGL::ManualLoop()
{
   
    

#ifdef CUICUI_IOS
    [uiOpenGlView AllowRender];
#endif
#ifdef CUICUI_UWP

	
	// skip this frame if another one is being rendered (or if the render loop is running)
	if (!mRenderSurfaceCriticalSection.try_lock()) return;
	struct UnlockAtEnd { critical_section &cs; ~UnlockAtEnd() { cs.unlock(); } } unlockAtEnd{ mRenderSurfaceCriticalSection };

	mOpenGLES->MakeCurrent(mRenderSurface);

	{

		mOpenGLES->GetSurfaceDimensions(mRenderSurface, &frameBufferWidth, &frameBufferHeight);

		if (firstFrame == true)
		{
			firstFrame = false;
			glThreadId = std::this_thread::get_id();
		}

		// Logic to update the scene could go here
        cui_assert(renderLoopCallback != nullptr);
		renderLoopCallback();


		// The call to eglSwapBuffers might not be successful (i.e. due to Device Lost)
		// If the call fails, then we must reinitialize EGL and the GL resources.
		if (mOpenGLES->SwapBuffers(mRenderSurface) != GL_TRUE)
		{
			// put back/test swapbuffers restore opengl
            cui_assert(false);

			return;
		}
	}
	
	
#endif
#ifdef CUICUI_ANDROID
	{
		JNIEnv* env = SafeJniEnv(globalCuiJniEnv);
		jclass c = jniCallManager.FindClass(env, wOpenGL_JavaClass);
		jmethodID m = wOpenGL_GetStaticMethod(env, c, "requestRenderFrame", "(Lio/github/mkdotcom/cuicui/CuicuiActivity;I)V");
		env->CallStaticVoidMethod(c, m, globalCuiJniObj, (jint)theId);
	}
#endif
}

#ifdef CUICUI_ANDROID
extern "C"
JNIEXPORT void
JNICALL
Java_io_github_mkdotcom_cuicui_wopengl_CuicuiWOpenGL_onDrawFrame(
		JNIEnv *env,
		jclass clazz,
		jlong widgetPtr
		)
{
	wOpenGL *w = (wOpenGL*)widgetPtr;
	if (w == nullptr) return;
	if (!CheckDefWidgetAlive(w)) return;

	if (w->firstFrame == true)
	{
		w->firstFrame = false;
		w->glThreadId = std::this_thread::get_id();
	}

	if (w->renderLoopCallback != nullptr)
	    w->renderLoopCallback();
}

extern "C"
JNIEXPORT void
JNICALL
Java_io_github_mkdotcom_cuicui_wopengl_CuicuiWOpenGL_onSurfaceChanged(
	JNIEnv * env,
	jclass clazz,
	jlong widgetPtr,
	int width,
	int height
)
{
	wOpenGL* w = (wOpenGL*)widgetPtr;
	if (w == nullptr) return;
	if (!CheckDefWidgetAlive(w)) return;

	if (w->firstFrame == true)
	{
		w->firstFrame = false;
		w->glThreadId = std::this_thread::get_id();
	}

	w->frameBufferWidth = width;
	w->frameBufferHeight = height;
	if (w->renderLoopCallback != nullptr)
		w->renderLoopCallback();
}
#endif

void wOpenGL::StartRenderLoop()
{
#ifdef CUICUI_IOS
    CUICUI_NYI // easy: "allowrender" just needs to always be true (we already use cadisplaylink which already handles the "vsync")
#endif
#ifdef CUICUI_UWP
	// Create a task for rendering that will be run on a background thread.
	auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler([this](Windows::Foundation::IAsyncAction ^ action)
	{
		// the render loop owns the render surface while it runs
		if (!mRenderSurfaceCriticalSection.try_lock()) return;
		struct UnlockAtEnd { critical_section &cs; ~UnlockAtEnd() { cs.unlock(); } } unlockAtEnd{ mRenderSurfaceCriticalSection };

		mOpenGLES->MakeCurrent(mRenderSurface);

		while (action->Status == Windows::Foundation::AsyncStatus::Started)
		{
			EGLint panelWidth = 0;
			EGLint panelHeight = 0;
			mOpenGLES->GetSurfaceDimensions(mRenderSurface, &panelWidth, &panelHeight);

			// Logic to update the scene could go here
            cui_assert(renderLoopCallback != nullptr);
			renderLoopCallback();
			

			// The call to eglSwapBuffers might not be successful (i.e. due to Device Lost)
			// If the call fails, then we must reinitialize EGL and the GL resources.
			if (mOpenGLES->SwapBuffers(mRenderSurface) != GL_TRUE)
			{
				// XAML objects like the SwapChainPanel must only be manipulated on the UI thread.
				swapChainPanel->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, ref new Windows::UI::Core::DispatchedHandler([=]()
				{
					/* TODO */ //RecoverFromLostDevice();
				}, CallbackContext::Any));

				return;
			}
		}
	});

	// Run task on a dedicated high priority background thread.
	auto mRenderLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High, Windows::System::Threading::WorkItemOptions::TimeSliced);
#endif
#ifdef CUICUI_ANDROID
	CUICUI_NYI
#endif
}

} // namespace cui