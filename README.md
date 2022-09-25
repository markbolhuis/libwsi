# libwsi

libwsi is a windowing abstraction library for Vulkan and EGL. It is designed to
be simple and easy to use. 

It is currently in development and is not yet ready for use.

## Architecture

libwsi is really a collection of libraries. The core library is `libwsi.so` and
is a thin dispatcher that forwards calls to the appropriate platform-specific
library, which is loaded at runtime.

Each platform-specific library implements the actual core functionality,
everything from creating a window to handling events.
They are named `libwsi-<platform>.so`, and link against appropriate system
libraries.

## Platforms

It aims to support the following platforms:

- Wayland
- X11
- Windows (help wanted)

Future platforms may include:

- Android
- macOS
- iOS

## License

libwsi is licensed under the Apache 2.0 license.

