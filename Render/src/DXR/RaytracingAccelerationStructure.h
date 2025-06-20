#pragma once

#include "../Resource/Device.h"
#include "../Resource/D3D12RHI.h"
#include "../Math/Transform.h"

#include <map>
#include <algorithm>
#ifdef max
#undef max
#endif

using namespace Microsoft::WRL;

struct AccelerationStructureBuffers
{
    ComPtr<ID3D12Resource> scratch;         // Temp work space
    ComPtr<ID3D12Resource> accelerationStructure;
    ComPtr<ID3D12Resource> instanceDesc;    // Used only for top-level AS
    UINT64                 ResultDataMaxSizeInBytes;
};

// AccelerationStructure
// A base class for bottom-level and top-level AS.
class AccelerationStructure
{
public:
    AccelerationStructure() {}
    virtual ~AccelerationStructure() {}
    void ReleaseD3DResources();
    UINT64 RequiredScratchSize() { return std::max(m_prebuildInfo.ScratchDataSizeInBytes, m_prebuildInfo.UpdateScratchDataSizeInBytes); }
    UINT64 RequiredResultDataSizeInBytes() { return m_prebuildInfo.ResultDataMaxSizeInBytes; }

    ASBufferRef GetASBuffer() const { return m_asBuffer; }
    ID3D12Resource* GetResource() const { return m_asBuffer ? m_asBuffer->GetResource()->D3DResource.Get() : nullptr; }
    ShaderResourceView* GetSRV() const { return m_asBuffer ? m_asBuffer->GetSRV() : nullptr; }

    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& PrebuildInfo() { return m_prebuildInfo; }
    const std::wstring& GetName() { return m_name; }

    void SetDirty(bool isDirty) { m_isDirty = isDirty; }
    bool IsDirty() { return m_isDirty; }
    UINT64 ResourceSize() { return GetResource()->GetDesc().Width; }

protected:
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuildInfo = {};
    std::wstring m_name;
    ASBufferRef m_asBuffer;

    bool m_isBuilt = false; // whether the AS has been built at least once.
    bool m_isDirty = true; // whether the AS has been modified and needs to be rebuilt.
    bool m_updateOnBuild = false;
    bool m_allowUpdate = false;

    void AllocateResource(D3D12RHI* d3dRHI);
};

// collect and organize geometry data for bottom-level AS construction
class BottomLevelAccelerationStructureGeometry
{
public:
    BottomLevelAccelerationStructureGeometry(
    const std::wstring& name,
    UINT vertexByteStride,
    DXGI_FORMAT vertexFormat,
    DXGI_FORMAT indexFormat,
    D3D12_RAYTRACING_GEOMETRY_FLAGS defaultFlags)
    : m_name(name),
      m_vertexByteStride(vertexByteStride),
      m_vertexFormat(vertexFormat),
      m_indexFormat(indexFormat),
      m_defaultFlags(defaultFlags){}

    // can use meshProxy
    void AddGeometry(
        VertexBufferRef vertexBuffer,
        IndexBufferRef indexBuffer,
        UINT vertexBufferByteSize,
        UINT indexBufferByteSize,
        D3D12_RAYTRACING_GEOMETRY_FLAGS flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE);

    const std::wstring& GetName() const { return m_name; }
    UINT GetGeometryCount() const { return static_cast<UINT>(m_geometryDescs.size()); }
    const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GetGeometryDescs() const { return m_geometryDescs; }

private:
    std::wstring m_name;
    
    UINT m_vertexByteStride;
    DXGI_FORMAT m_vertexFormat;
    DXGI_FORMAT m_indexFormat;
    D3D12_RAYTRACING_GEOMETRY_FLAGS m_defaultFlags;
    
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
    std::vector<VertexBufferRef> m_vertexBuffers;
    std::vector<IndexBufferRef> m_indexBuffers;
};

class BottomLevelAccelerationStructure : public AccelerationStructure
{
public:
    BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(const std::wstring& name, UINT frameCount)
    : AccelerationStructure(), m_frameCount(frameCount){}
    ~BottomLevelAccelerationStructure() {}

    void Initialize(
        D3D12RHI* d3dRHI, 
	    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, 
        BottomLevelAccelerationStructureGeometry& bottomLevelASGeometry, 
        bool allowUpdate,
        bool bUpdateOnBuild);
        
    void Build(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* scratch, ID3D12DescriptorHeap* descriptorHeap, D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress = 0);

    void UpdateGeometryDescsTransform(D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress);

    UINT GetInstanceContributionToHitGroupIndex() { return m_instanceContributionToHitGroupIndex; }
    void SetInstanceContributionToHitGroupIndex(UINT index) { m_instanceContributionToHitGroupIndex = index; }

    const TTransform& GetTransform() { return m_transform; }
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GetGeometryDescs() { return m_geometryDescs; }

private:
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
    UINT currentID = 0;
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_cacheGeometryDescs[3];

    UINT m_frameCount;
    TTransform m_transform;
    UINT m_instanceContributionToHitGroupIndex = 0;
    
    void BuildGeometryDescs(BottomLevelAccelerationStructureGeometry& bottomLevelASGeometry);
    void ComputePrebuildInfo(D3D12RHI* d3dRHI);
};

class TopLevelAccelerationStructure : public AccelerationStructure
{
public:
    TopLevelAccelerationStructure() {}
    ~TopLevelAccelerationStructure() {}

    void Initialize(D3D12RHI* d3dRHI, UINT numBottomLevelASInstanceDescs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, bool allowUpdate = false, bool bUpdateOnBuild = false, const wchar_t* resourceName = nullptr);
    void Build(ID3D12GraphicsCommandList4* commandList, UINT numInstanceDescs, D3D12_GPU_VIRTUAL_ADDRESS InstanceDescs, ID3D12Resource* scratch, ID3D12DescriptorHeap* descriptorHeap, bool bUpdate = false);

private:
    void ComputePrebuildInfo(D3D12RHI* d3dRHI, UINT numBottomLevelASInstanceDescs);
};

class RaytracingAccelerationStructureManager
{
public:
    RaytracingAccelerationStructureManager(D3D12RHI* d3dRHI, UINT numBottomLevelInstances, UINT frameCount);
    ~RaytracingAccelerationStructureManager()
    {
        if (m_mappedInstanceData && m_bottomLevelASInstanceDescs)
        {
            m_bottomLevelASInstanceDescs->GetResource()->D3DResource->Unmap(0, nullptr);
            m_mappedInstanceData = nullptr;
        }
    }

    void AddBottomLevelAS(D3D12RHI* d3dRHI, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, BottomLevelAccelerationStructureGeometry& bottomLevelASGeometry, bool allowUpdate = false, bool performUpdateOnBuild = false);
    UINT AddBottomLevelASInstance(const std::wstring& bottomLevelASname, UINT instanceContributionToHitGroupIndex = UINT_MAX, TTransform transform, BYTE InstanceMask = 1);
    D3D12_RAYTRACING_INSTANCE_DESC& GetBottomLevelASInstance(UINT index);
    void InitializeTopLevelAS(D3D12RHI* d3dRHI, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, bool allowUpdate = false, bool performUpdateOnBuild = false, const wchar_t* resourceName = nullptr);
    void Build(D3D12RHI* d3dRHI, ID3D12GraphicsCommandList4* commandList, ID3D12DescriptorHeap* descriptorHeap, UINT frameIndex, bool bForceBuild = false);
    const StructuredBufferRef GetBottomLevelASInstancesBuffer() { return m_bottomLevelASInstanceDescs; }
    UINT GetNumberOfBottomLevelASInstances() {return static_cast<UINT>(m_instanceDescs.size());}
    BottomLevelAccelerationStructure& GetBottomLevelAS(const std::wstring& name) { return m_vBottomLevelAS[name]; }
    ID3D12Resource* GetTopLevelASResource() { return m_topLevelAS.GetResource(); }
    UINT64 GetASMemoryFootprint() { return m_ASmemoryFootprint; }
    UINT GetMaxInstanceContributionToHitGroupIndex();

private:
    TopLevelAccelerationStructure m_topLevelAS;
    std::map<std::wstring, BottomLevelAccelerationStructure> m_vBottomLevelAS;
    StructuredBufferRef m_bottomLevelASInstanceDescs;
    UINT m_numBottomLevelASInstances = 0;
    ComPtr<ID3D12Resource>	m_accelerationStructureScratch;
    UINT64 m_scratchResourceSize = 0;
    UINT64 m_ASmemoryFootprint = 0;

    void RaytracingAccelerationStructureManager::SetInstanceTransform(D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc, const TTransform& transform)
    {
        TMatrix transformMatrix = transform.GetTransformMatrix();
        DirectX::XMMATRIX xmMatrix = transformMatrix;
        DirectX::XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instanceDesc.Transform), xmMatrix);
    }

    D3D12_RAYTRACING_INSTANCE_DESC* m_mappedInstanceData = nullptr;
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
};