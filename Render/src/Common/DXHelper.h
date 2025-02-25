//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
#include "stdafx.h"
#include <stdexcept>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
using Microsoft::WRL::ComPtr;

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<uint32_t>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline void GetAssetsPath(_Out_writes_(pathSize) wchar_t* path, uint32_t pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

inline HRESULT ReadDataFromFile(LPCWSTR filename, uint8_t** data, uint32_t* size)
{
    using namespace Microsoft::WRL;

#if WINVER >= _WIN32_WINNT_WIN8
    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
#else
    Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif
    if (file.Get() == INVALID_HANDLE_VALUE)
    {
        throw std::exception();
    }

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        throw std::exception();
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        throw std::exception();
    }

    *data = reinterpret_cast<uint8_t*>(malloc(fileInfo.EndOfFile.LowPart));
    *size = fileInfo.EndOfFile.LowPart;

    if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        throw std::exception();
    }

    return S_OK;
}

inline HRESULT ReadDataFromDDSFile(LPCWSTR filename, uint8_t** data, uint32_t* offset, uint32_t* size)
{
    if (FAILED(ReadDataFromFile(filename, data, size)))
    {
        return E_FAIL;
    }

    // DDS files always start with the same magic number.
    static const uint32_t DDS_MAGIC = 0x20534444;
    uint32_t magicNumber = *reinterpret_cast<const uint32_t*>(*data);
    if (magicNumber != DDS_MAGIC)
    {
        return E_FAIL;
    }

    struct DDS_PIXELFORMAT
    {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t rgbBitCount;
        uint32_t rBitMask;
        uint32_t gBitMask;
        uint32_t bBitMask;
        uint32_t aBitMask;
    };

    struct DDS_HEADER
    {
        uint32_t size;
        uint32_t flags;
        uint32_t height;
        uint32_t width;
        uint32_t pitchOrLinearSize;
        uint32_t depth;
        uint32_t mipMapCount;
        uint32_t reserved1[11];
        DDS_PIXELFORMAT ddsPixelFormat;
        uint32_t caps;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
        uint32_t reserved2;
    };

    auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(uint32_t));
    if (ddsHeader->size != sizeof(DDS_HEADER) || ddsHeader->ddsPixelFormat.size != sizeof(DDS_PIXELFORMAT))
    {
        return E_FAIL;
    }

    const ptrdiff_t ddsDataOffset = sizeof(uint32_t) + sizeof(DDS_HEADER);
    *offset = ddsDataOffset;
    *size = *size - ddsDataOffset;

    return S_OK;
}

// Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, uint32_t index)
{
    wchar_t fullName[50];
    if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
    {
        pObject->SetName(fullName);
    }
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, uint32_t)
{
}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

inline uint32_t CalculateConstantBufferByteSize(uint32_t byteSize)
{
    // Constant buffer size is required to be aligned.
    return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    uint32_t compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr;

    Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    return byteCode;
}
#endif

// Resets all elements in a ComPtr array.
template<class T>
void ResetComPtrArray(T* comPtrArray)
{
    for (auto& i : *comPtrArray)
    {
        i.Reset();
    }
}


// Resets all elements in a unique_ptr array.
template<class T>
void ResetUniquePtrArray(T* uniquePtrArray)
{
    for (auto& i : *uniquePtrArray)
    {
        i.reset();
    }
}

// 为 WICPixelFormatGUID 特化哈希函数
namespace std {
	template <>
	struct hash<WICPixelFormatGUID> {
		size_t operator()(const WICPixelFormatGUID& guid) const {
			// 哈希函数通过组合 GUID 的各个字段来生成哈希值
			size_t hashValue = 0;
			// 使用标准的哈希函数对 GUID 的各个字段进行哈希处理
			hashValue ^= hash<uint32_t>()(guid.Data1) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
			hashValue ^= hash<uint16_t>()(guid.Data2) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
			hashValue ^= hash<uint16_t>()(guid.Data3) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
			// 处理 Data4 的 8 字节数组
			for (int i = 0; i < 8; ++i) {
				hashValue ^= hash<uint8_t>()(guid.Data4[i]) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
			}
			return hashValue;
		}
	};
}
// DXGI_FORMAT 是 GPU 层面的资源格式，用于优化硬件性能和图形渲染。
inline DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	// 静态映射表，将 WICPixelFormatGUID 映射到 DXGI_FORMAT
	static const std::unordered_map<WICPixelFormatGUID, DXGI_FORMAT> formatMap = {
		{ GUID_WICPixelFormat128bppRGBAFloat, DXGI_FORMAT_R32G32B32A32_FLOAT },
		{ GUID_WICPixelFormat64bppRGBAHalf, DXGI_FORMAT_R16G16B16A16_FLOAT },
		{ GUID_WICPixelFormat64bppRGBA, DXGI_FORMAT_R16G16B16A16_UNORM },
		{ GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGRA, DXGI_FORMAT_B8G8R8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGR, DXGI_FORMAT_B8G8R8X8_UNORM },
		{ GUID_WICPixelFormat32bppRGBA1010102XR, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
		{ GUID_WICPixelFormat32bppRGBA1010102, DXGI_FORMAT_R10G10B10A2_UNORM },
		{ GUID_WICPixelFormat16bppBGRA5551, DXGI_FORMAT_B5G5R5A1_UNORM },
		{ GUID_WICPixelFormat16bppBGR565, DXGI_FORMAT_B5G6R5_UNORM },
		{ GUID_WICPixelFormat32bppGrayFloat, DXGI_FORMAT_R32_FLOAT },
		{ GUID_WICPixelFormat16bppGrayHalf, DXGI_FORMAT_R16_FLOAT },
		{ GUID_WICPixelFormat16bppGray, DXGI_FORMAT_R16_UNORM },
		{ GUID_WICPixelFormat8bppGray, DXGI_FORMAT_R8_UNORM },
		{ GUID_WICPixelFormat8bppAlpha, DXGI_FORMAT_A8_UNORM }
	};

	// 在映射表中查找对应的 DXGI_FORMAT
	auto it = formatMap.find(wicFormatGUID);
	if (it != formatMap.end()) {
		return it->second;  // 找到映射，返回对应的 DXGI_FORMAT
	}

	// 如果未找到对应的格式，返回 DXGI_FORMAT_UNKNOWN
	return DXGI_FORMAT_UNKNOWN;
}

// WICPixelFormatGUID 是针对图像文件处理的高级抽象，主要用于 CPU 层。
inline WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	static const std::unordered_map<WICPixelFormatGUID, WICPixelFormatGUID> formatMap = {
		{ GUID_WICPixelFormatBlackWhite, GUID_WICPixelFormat8bppGray },
		{ GUID_WICPixelFormat1bppIndexed, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat2bppIndexed, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat4bppIndexed, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat8bppIndexed, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat2bppGray, GUID_WICPixelFormat8bppGray },
		{ GUID_WICPixelFormat4bppGray, GUID_WICPixelFormat8bppGray },
		{ GUID_WICPixelFormat16bppGrayFixedPoint, GUID_WICPixelFormat16bppGrayHalf },
		{ GUID_WICPixelFormat32bppGrayFixedPoint, GUID_WICPixelFormat32bppGrayFloat },
		{ GUID_WICPixelFormat16bppBGR555, GUID_WICPixelFormat16bppBGRA5551 },
		{ GUID_WICPixelFormat32bppBGR101010, GUID_WICPixelFormat32bppRGBA1010102 },
		{ GUID_WICPixelFormat24bppBGR, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat24bppRGB, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat32bppPBGRA, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat32bppPRGBA, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat48bppRGB, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat48bppBGR, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat64bppBGRA, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat64bppPRGBA, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat64bppPBGRA, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat48bppRGBFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat48bppBGRFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat64bppRGBAFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat64bppBGRAFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat64bppRGBFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat64bppRGBHalf, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat48bppRGBHalf, GUID_WICPixelFormat64bppRGBAHalf },
		{ GUID_WICPixelFormat128bppPRGBAFloat, GUID_WICPixelFormat128bppRGBAFloat },
		{ GUID_WICPixelFormat128bppRGBFloat, GUID_WICPixelFormat128bppRGBAFloat },
		{ GUID_WICPixelFormat128bppRGBAFixedPoint, GUID_WICPixelFormat128bppRGBAFloat },
		{ GUID_WICPixelFormat128bppRGBFixedPoint, GUID_WICPixelFormat128bppRGBAFloat },
		{ GUID_WICPixelFormat32bppRGBE, GUID_WICPixelFormat128bppRGBAFloat },
		{ GUID_WICPixelFormat32bppCMYK, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat64bppCMYK, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat40bppCMYKAlpha, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat80bppCMYKAlpha, GUID_WICPixelFormat64bppRGBA },
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
		{ GUID_WICPixelFormat32bppRGB, GUID_WICPixelFormat32bppRGBA },
		{ GUID_WICPixelFormat64bppRGB, GUID_WICPixelFormat64bppRGBA },
		{ GUID_WICPixelFormat64bppPRGBAHalf, GUID_WICPixelFormat64bppRGBAHalf },
#endif
	};

	// 查找输入格式并返回相应的目标格式
	auto it = formatMap.find(wicFormatGUID);
	if (it != formatMap.end()) {
		return it->second;  // 找到映射，返回对应的目标格式
	}

	// 如果没有找到映射，则返回 GUID_WICPixelFormatDontCare
	return GUID_WICPixelFormatDontCare;
}


inline int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
	// 静态映射表，将 DXGI_FORMAT 映射到每像素位数
	static const std::unordered_map<DXGI_FORMAT, int> formatBitsPerPixelMap = {
		{ DXGI_FORMAT_R32G32B32A32_FLOAT, 128 },
		{ DXGI_FORMAT_R16G16B16A16_FLOAT, 64 },
		{ DXGI_FORMAT_R16G16B16A16_UNORM, 64 },
		{ DXGI_FORMAT_R8G8B8A8_UNORM, 32 },
		{ DXGI_FORMAT_B8G8R8A8_UNORM, 32 },
		{ DXGI_FORMAT_B8G8R8X8_UNORM, 32 },
		{ DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 32 },
		{ DXGI_FORMAT_R10G10B10A2_UNORM, 32 },
		{ DXGI_FORMAT_B5G5R5A1_UNORM, 16 },
		{ DXGI_FORMAT_B5G6R5_UNORM, 16 },
		{ DXGI_FORMAT_R32_FLOAT, 32 },
		{ DXGI_FORMAT_R16_FLOAT, 16 },
		{ DXGI_FORMAT_R16_UNORM, 16 },
		{ DXGI_FORMAT_R8_UNORM, 8 },
		{ DXGI_FORMAT_A8_UNORM, 8 }
	};

	// 查找映射表中的对应位数
	auto it = formatBitsPerPixelMap.find(dxgiFormat);
	if (it != formatBitsPerPixelMap.end()) {
		return it->second;  // 返回找到的位数
	}

	// 如果未找到对应的格式，返回 0
	return 0;
}

// 加载图片

inline int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow, uint& textureWidth, uint& textureHeight, DXGI_FORMAT& dxgiFormat)
{
	HRESULT hr;

	static IWICImagingFactory* wicFactory;

	IWICBitmapDecoder* wicDecoder = NULL;
	IWICBitmapFrameDecode* wicFrame = NULL;
	IWICFormatConverter* wicConverter = NULL;

	bool imageConverted = false;

	// 初始化 WIC 工厂
	if (wicFactory == NULL)
	{
		CoInitialize(NULL);

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&wicFactory)
		);
		if (FAILED(hr)) return 0;

		hr = wicFactory->CreateFormatConverter(&wicConverter);
		if (FAILED(hr)) return 0;
	}

	// 解码图像文件
	hr = wicFactory->CreateDecoderFromFilename(
		filename,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&wicDecoder
	);
	if (FAILED(hr)) return 0;

	// 获取帧和像素格式
	hr = wicDecoder->GetFrame(0, &wicFrame);
	if (FAILED(hr)) return 0;

	WICPixelFormatGUID pixelFormat;
	hr = wicFrame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr)) return 0;

	hr = wicFrame->GetSize(&textureWidth, &textureHeight);
	if (FAILED(hr)) return 0;

	// 转换为 DirectX 支持的格式
	dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

	if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

		if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

		dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

		BOOL canConvert = FALSE;
		hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
		if (FAILED(hr) || !canConvert) return 0;

		hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) return 0;

		imageConverted = true;
	}

	// 分配内存并拷贝像素数据
	int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
	bytesPerRow = (textureWidth * bitsPerPixel) / 8;
	int imageSize = bytesPerRow * textureHeight;

	*imageData = (BYTE*)malloc(imageSize);

	if (imageConverted)
	{
		hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (FAILED(hr)) return 0;
	}
	else
	{
		hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (FAILED(hr)) return 0;
	}

	// 设置 D3D12 资源描述
	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0;
	resourceDescription.Width = textureWidth;
	resourceDescription.Height = textureHeight;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.Format = dxgiFormat;
	resourceDescription.SampleDesc.Count = 1;
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

	return imageSize;
}