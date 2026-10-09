# Vulkan Ray Tracing Engine

Motor de ray tracing multiplataforma con dos backends de cómputo:
**CUDA + Vulkan** en Windows y **Metal + Vulkan** en macOS. La misma base
de código corre en ambas plataformas; solo cambia el backend de cómputo y
la capa de ventana, aislados en archivos específicos de plataforma.

## Vista previa

![Test render](test_render.png)

## Características

- **Path tracing progresivo** con acumulación temporal y Russian Roulette.
- **BVH con pruning exacto** para traversal eficiente de la escena.
- **Primitivas soportadas:** esferas, triángulos, cajas y cilindros.
- **Tonemapping HDR → LDR** con dithering estocástico.
- **Cero copias en macOS:** gracias a la memoria unificada de Apple Silicon,
  el kernel Metal escribe directamente en el buffer de Vulkan.
- **Ventana nativa** en cada plataforma (Win32 / Cocoa), sin dependencias
  externas como GLFW o SDL.
- **Resize, fullscreen y captura de mouse** funcionando en ambas plataformas.

## Plataformas soportadas

| Plataforma | Cómputo | API gráfica | Driver Vulkan | Estado |
|---|---|---|---|---|
| Windows | CUDA | Vulkan | Nativo (NVIDIA/AMD) | ✅ |
| macOS (Apple Silicon) | Metal | Vulkan | KosmicKrisp o MoltenVK | ✅ |
| macOS (Intel) | — | — | — | ❌ |

> **Nota sobre macOS Intel:** el motor asume memoria unificada y usa la API
> Metal para cómputo. No funcionará en Macs Intel con GPU discreta.

## Requisitos

### Windows

- **Visual Studio 2022** (con soporte C++, MSVC, y compilador CUDA).
- **CUDA Toolkit 12.x** o superior.
- **Vulkan SDK** de [LunarG](https://vulkan.lunarg.com/sdk/home).
- **CMake 3.21+** y **Ninja** (opcional).
- **GPU NVIDIA** compatible con CUDA.

### macOS (Apple Silicon)

- **Xcode 16+** con **Metal Toolchain** instalado:
  ```bash
  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
  sudo xcodebuild -runFirstLaunch
  xcodebuild -downloadComponent MetalToolchain