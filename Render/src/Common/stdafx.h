#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN// Exclude rarely-used stuff from Windows headers.
#endif
#define NOMINMAX
#include <windows.h>
#include <stdint.h>

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <string>
#include <wrl.h>
#include <shellapi.h>
#include "Metalib.h"
#include <span>
#include <array>

#include <wincodec.h>
#include <unordered_map>

#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include "Assimp/postprocess.h"