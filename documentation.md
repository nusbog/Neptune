# Neptune - Documentation
## Overview
The [neptune_core.cpp](src/neptune_core.cpp) script contains only rendering and window managment such as render textures,
models, callback function handling, shader handling, window resizing, etc. <br>
The [main.cpp](src/main.cpp) script is the actual program that the user sees on startup, it contains calls to
rendering features located in the core framework.

## Callbacks
Callback function initialization is listed at the bottom of the [neptune_core.h](include/neptune_core.h) file.
The following will initialize a callback to a function which will render objects, where "onPaint" is a basic function returning void:
```c++
    neptune_onPaintCallback(onPaint);
```

## Shaders
Anything that is drawn to the screen needs to be attached to a shader written in GLSL.
This is the procedure to initialize a shader:
```c++
    Shader basicShader;
    basicShader = neptune_loadShader("vertexPath.vs", "fragmentPath.fs");
    basicShader.getShaderUniformLocations();
    basicShader.pushMatricesToShader();
```

## Primitives
Shape drawing functions (NRectangle and NImage) inherit from the NPrimitive class.
This is the procedure to draw a simple rectangle:
```c++
    NRectangle rect;
    rect.init(0.0f, 0.0f, 100.0f, 100.0f);
    rect.setShader(&basicShader);

    // ...In OnPaint callback
    rect.draw();
```