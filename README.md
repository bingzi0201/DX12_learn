# DX12_learn
## Project Description

This project is a **real-time renderer built on DirectX 12 (DX12)**. Core features include:
*   **Buddy Allocation** for efficient GPU resource management.
*   **Assimp** library integration for mesh asset importing.
*   Support for **Rasterization-based shading** and **Compute Shaders**.
*   Partial **DirectX Raytracing (DXR)** extensions.

### Currently Implemented Features

1.  **PBR Direct Lighting:**
    *   **Diffuse:** Lambertian BRDF model.
    *   **Normal Distribution Function (NDF):** GGX / Trowbridge-Reitz model.
    *   **Geometry Function:** Schlick-GGX model (k remapped to alpha/2 to approximate GGX Smith).
    *   **Fresnel:** Spherical Gaussian (SG) approximation.
2.  **Image-Based Lighting (IBL):**
    *   **Diffuse:** Irradiance environment map.
    *   **Specular:** Pre-filtered environment map (mipmapped).
3.  **Compute Shader:** Environment map CDF (Cumulative Distribution Function) calculation via compute shaders.
4.  **DXR Acceleration Structures:** Implementation of ray tracing acceleration structures.
