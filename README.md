# **Fossil Threads by Fossil Logic**

**Fossil Threads** is a lightweight, portable multithreading library written in pure C with no external dependencies. Designed for cross-platform applications, embedded systems, and performance-critical software, Fossil Threads provides simple, efficient abstractions for thread creation, synchronization, and management while maintaining minimal footprint and maximum portability.

### Key Features

- **Cross-Platform Support**  
  Works reliably on Windows, macOS, Linux, and embedded platforms.

- **Zero External Dependencies**  
  Written entirely in C for maximum portability, auditability, and ease of integration.

- **Thread Abstraction**  
  Simple APIs for creating, joining, and managing threads across platforms.

- **Synchronization Primitives**  
  Includes mutexes, condition variables, semaphores, and lightweight locks for safe concurrent programming.

- **Lightweight and Efficient**  
  Optimized for minimal resource usage while maintaining high performance.

- **Modular Design**  
  Easily extended or integrated into existing projects without imposing heavy dependencies.

## Getting Started

### Prerequisites

- **Meson Build System**  
  Fossil Threads uses Meson for build configuration. If you don’t have Meson installed, follow the instructions on the official [Meson website](https://mesonbuild.com/Getting-meson.html).

### Adding Fossil Threads as a Dependency

#### Using Meson

### **Install or Upgrade Meson** (version 1.3 or newer recommended):

```sh
   python -m pip install meson           # Install Meson
   python -m pip install --upgrade meson # Upgrade Meson
```
###	Add the .wrap File
Place a file named fossil-threads.wrap in your subprojects directory with the following content:

```ini
# ======================
# Git Wrap package definition
# ======================
[wrap-git]
url = https://github.com/fossillogic/fossil-threads.git
revision = v0.1.0

[provide]
fossil-threads = fossil_threads_dep
```

###	Integrate in Your meson.build
Add the dependency by including this line:

```meson
threads_dep = dependency('fossil-threads')
```


## Build Configuration Options

Customize your build with the following Meson options:
	•	Enable Tests
To run the built-in test suite, configure Meson with:

```sh
meson setup builddir -Dwith_test=enabled
```

## Contributing and Support

For those interested in contributing, reporting issues, or seeking support, please open an issue on the project repository or visit the [Fossil Logic Docs](https://fossillogic.com/docs) for more information. Your feedback and contributions are always welcome.