# cuicui_wOpenGL

OpenGL ES widget module for [cuicui](https://github.com/mkdotcom/cuicui_core): a native OpenGL ES
surface inside your cuicui layout.

> [!WARNING]
> **Status: experimental.** API not stable.
>
> | Platform | Backend | Status |
> |---|---|---|
> | Android | `GLSurfaceView` (OpenGL ES 2) | tested; manual render loop only (`StartRenderLoop()` not implemented) |
> | iOS | OpenGL ES (`EAGLContext`) | worked before the cleanup for the public release, not rebuilt since; OpenGL ES is deprecated by Apple and not available in Mac Catalyst apps |
> | UWP | ANGLE (OpenGL ES over Direct3D) | worked before the cleanup for the public release, not rebuilt since |
> | Web | – | not implemented yet (empty widget, a warning is logged) |
>
> `wOpenGL::IsSupported()` tells if the widget works on the current platform.

## Setup

Clone this repository into the `modules` folder of your app:

```
cd modules
git clone https://github.com/mkdotcom/cuicui_wOpenGL.git
```

Add `modules/cuicui_wOpenGL/wOpenGL.cpp` to your build, then:

```cpp
#include "modules/cuicui_wOpenGL/wOpenGL.h"
```

### Android
In `CMakeLists.txt`:
```cmake
target_sources(native-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/cuicui_wOpenGL/wOpenGL.cpp)
target_link_libraries(native-lib GLESv2 EGL)
```
In `app/build.gradle`, add the module's Java code:
```gradle
sourceSets {
    main.java.srcDirs += '../../../modules/cuicui_wOpenGL/android/'
}
```

### iOS
Add `wOpenGL.cpp` to the Xcode project (as Objective-C++, like the other cuicui files) and link
`OpenGLES.framework`, `GLKit.framework` and `QuartzCore.framework`.

### UWP
Add the NuGet package `ANGLE.WindowsStore` to your app project.

## Usage

```cpp
#include "modules/cuicui_wOpenGL/wOpenGL.h"

wOpenGL* ogl = wOpenGL::Create(FILL, ems(10));
ogl->SetManualRenderLoopCallback([]()
{
	glClearColor(0.0f, 0.8f, 0.2f, 1.0f); // green
	glClear(GL_COLOR_BUFFER_BIT);
	// your OpenGL ES code here
});
ogl->BgColor(0x800080); // purple: visible only if nothing is rendered
dc += ogl;

dc += wButton::Create(MIN, ems(1.6), "Redraw", ems(0.6))->OnClick([ogl](wButton*) { ogl->ManualLoop(); });
```

- `ManualLoop()` asks for one frame; your callback is called with the OpenGL context current.
- **Threads:** on Android the callback runs on the OpenGL thread, not on the UI thread.
- `GetFrameBufferWidth()` / `GetFrameBufferHeight()`: size of the drawing surface, in pixels.
- `SetRenderLoopCallback()` + `StartRenderLoop()`: continuous rendering (UWP only for now).

If you see green: it works. If you see purple: nothing was rendered.

## License

MIT — see [LICENSE](LICENSE).
