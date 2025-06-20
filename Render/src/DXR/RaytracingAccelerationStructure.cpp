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

#include "RaytracingAccelerationStructure.h"
#include "../Utils/D3D12Utils.h"
#include <stdexcept>

using namespace std;


void AccelerationStructure::ReleaseD3DResources()
{
    m_asBuffer->GetResource()->D3DResource.Reset();
}

void AccelerationStructure::AllocateResource(D3D12RHI* d3dRHI)
{
    m_asBuffer = d3dRHI->CreateTopLevelAccelerationStructure(m_prebuildInfo.ResultDataMaxSizeInBytes, m_name.c_str());
}

void BottomLevelAccelerationStructure::Initialize(
	D3D12RHI* d3dRHI, 
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, 
    BottomLevelAccelerationStructureGeometry& bottomLevelASGeometry, 
    bool allowUpdate,
    bool bUpdateOnBuild)
{
    m_allowUpdate = allowUpdate;
    m_updateOnBuild = bUpdateOnBuild;

    m_buildFlags = buildFlags;
    m_name = bottomLevelASGeometry.GetName();
    
    if (allowUpdate)
    {
        m_buildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }

	BuildGeometryDescs(bottomLevelASGeometry);
	ComputePrebuildInfo(d3dRHI);
	AllocateResource(d3dRHI);

	m_isDirty = true;
    m_isBuilt = false;
}

void BottomLevelAccelerationStructureGeometry::AddGeometry(
    VertexBufferRef vertexBuffer,
    IndexBufferRef indexBuffer,
    UINT vertexBufferByteSize,
    UINT indexBufferByteSize,
    D3D12_RAYTRACING_GEOMETRY_FLAGS flags)
{
    if (!vertexBuffer || !indexBuffer || !vertexBuffer->GetResource() || !indexBuffer->GetResource() ||
        m_vertexByteStride == 0 || vertexBufferByteSize == 0 || indexBufferByteSize == 0)
    {
        return;
    }

    // if the flags are not set, use the default flags.
    if (flags == D3D12_RAYTRACING_GEOMETRY_FLAG_NONE) {
        flags = m_defaultFlags;
    }

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Flags = flags;
    
    geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetResource()->D3DResource->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = m_vertexByteStride;
    geometryDesc.Triangles.VertexCount = vertexBufferByteSize / m_vertexByteStride;
    geometryDesc.Triangles.VertexFormat = m_vertexFormat;
    
    geometryDesc.Triangles.IndexBuffer = indexBuffer->GetResource()->D3DResource->GetGPUVirtualAddress();
    UINT indexSize = (m_indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4;
    geometryDesc.Triangles.IndexCount = indexBufferByteSize / indexSize;
    geometryDesc.Triangles.IndexFormat = m_indexFormat;
    
    geometryDesc.Triangles.Transform3x4 = 0; // No transform by default, can be updated later.
    
    m_geometryDescs.push_back(geometryDesc);
    
    // remain the vertex and index buffers for later use.
    m_vertexBuffers.push_back(vertexBuffer);
    m_indexBuffers.push_back(indexBuffer);
}

void BottomLevelAccelerationStructure::UpdateGeometryDescsTransform(D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress)
{
    struct alignas(16) AlignedGeometryTransform3x4
    {
        float transform3x4[12];
    };

    for (UINT i = 0; i < m_geometryDescs.size(); i++)
    {
        auto& geometryDesc = m_geometryDescs[i];
        geometryDesc.Triangles.Transform3x4 = baseGeometryTransformGPUAddress + i * sizeof(AlignedGeometryTransform3x4);
    }
}

// Build geometry descs for bottom-level AS.
void BottomLevelAccelerationStructure::BuildGeometryDescs(BottomLevelAccelerationStructureGeometry& bottomLevelASGeometry)
{
    const auto& sourceDescs = bottomLevelASGeometry.GetGeometryDescs();
    
    m_geometryDescs.reserve(sourceDescs.size());
    m_geometryDescs = sourceDescs;
    
    if (m_geometryDescs.empty())
    {
        throw std::runtime_error("No geometry descriptions available in BottomLevelAccelerationStructureGeometry.");
    }
}

void BottomLevelAccelerationStructure::ComputePrebuildInfo(D3D12RHI* d3dRHI)
{
    // Get the size requirements for the scratch and AS buffers.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = m_buildFlags;
    bottomLevelInputs.NumDescs = static_cast<UINT>(m_geometryDescs.size());
    bottomLevelInputs.pGeometryDescs = m_geometryDescs.data();

    d3dRHI->GetDevice()->GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &m_prebuildInfo);
    ThrowIfFalse(m_prebuildInfo.ResultDataMaxSizeInBytes > 0);
}

// The caller must add a UAV barrier before using the resource.
void BottomLevelAccelerationStructure::Build(
    ID3D12GraphicsCommandList4* commandList,
    ID3D12Resource* scratch,
    ID3D12DescriptorHeap* descriptorHeap,
    D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress)
{
    ThrowIfFalse(m_prebuildInfo.ScratchDataSizeInBytes <= scratch->GetDesc().Width, L"Insufficient scratch buffer size provided!");

    if (baseGeometryTransformGPUAddress > 0)
    {
        UpdateGeometryDescsTransform(baseGeometryTransformGPUAddress);
    }

    currentID = (currentID + 1) % m_frameCount;
    m_cacheGeometryDescs[currentID].clear();
    m_cacheGeometryDescs[currentID].resize(m_geometryDescs.size());
    copy(m_geometryDescs.begin(), m_geometryDescs.end(), m_cacheGeometryDescs[currentID].begin());

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    {
        bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        bottomLevelInputs.Flags = m_buildFlags;
        if (m_isBuilt && m_allowUpdate && m_updateOnBuild)
        {
            bottomLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
            bottomLevelBuildDesc.SourceAccelerationStructureData = GetResource()->GetGPUVirtualAddress();
        }
        bottomLevelInputs.NumDescs = static_cast<UINT>(m_cacheGeometryDescs[currentID].size());
        bottomLevelInputs.pGeometryDescs = m_cacheGeometryDescs[currentID].data();

        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = GetResource()->GetGPUVirtualAddress();
    }

    commandList->SetDescriptorHeaps(1, &descriptorHeap);
    commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

    m_isDirty = false;
    m_isBuilt = true;
}

void TopLevelAccelerationStructure::ComputePrebuildInfo(D3D12RHI* d3dRHI, UINT numBottomLevelASInstanceDescs)
{
    // Get the size requirements for the scratch and AS buffers.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = m_buildFlags;
    topLevelInputs.NumDescs = numBottomLevelASInstanceDescs;

    d3dRHI->GetDevice()->GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &m_prebuildInfo);
    ThrowIfFalse(m_prebuildInfo.ResultDataMaxSizeInBytes > 0);
}

void TopLevelAccelerationStructure::Initialize(
    D3D12RHI* d3dRHI,
    UINT numBottomLevelASInstanceDescs,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags,
    bool allowUpdate,
    bool bUpdateOnBuild,
    const wchar_t* resourceName)
{
    m_allowUpdate = allowUpdate;
    m_updateOnBuild = bUpdateOnBuild;
    m_buildFlags = buildFlags;
    m_name = resourceName;

    if (allowUpdate)
    {
        m_buildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }

    ComputePrebuildInfo(d3dRHI, numBottomLevelASInstanceDescs);
    AllocateResource(d3dRHI);

    m_isDirty = true;
    m_isBuilt = false;
}

void TopLevelAccelerationStructure::Build(ID3D12GraphicsCommandList4* commandList, UINT numBottomLevelASInstanceDescs, D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASnstanceDescs, ID3D12Resource* scratch, ID3D12DescriptorHeap* descriptorHeap, bool bUpdate)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    {
        topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        topLevelInputs.Flags = m_buildFlags;
        if (m_isBuilt && m_allowUpdate && m_updateOnBuild)
        {
            topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        }
        topLevelInputs.NumDescs = numBottomLevelASInstanceDescs;

        topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
        topLevelBuildDesc.DestAccelerationStructureData = GetResource()->GetGPUVirtualAddress();
    }
    topLevelInputs.InstanceDescs = bottomLevelASnstanceDescs;

    commandList->SetDescriptorHeaps(1, &descriptorHeap);
    commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    m_isDirty = false;
    m_isBuilt = true;
}

RaytracingAccelerationStructureManager::RaytracingAccelerationStructureManager(D3D12RHI* d3dRHI, UINT numBottomLevelInstances, UINT frameCount)
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> emptyInstances(numBottomLevelInstances);
    m_bottomLevelASInstanceDescs = d3dRHI->CreateStructuredBuffer(emptyInstances.data(),
        sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
        numBottomLevelInstances);

    if (m_bottomLevelASInstanceDescs)
    {
        m_mappedInstanceData = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(
            m_bottomLevelASInstanceDescs->GetResource()->mappedBaseAddress);
    }
}

// Adds a bottom-level Acceleration Structure.
// The passed in bottom-level AS geometry must have a unique name.
// Requires a corresponding 1 or more AddBottomLevelASInstance() calls to be added to the top-level AS for the bottom-level AS to be included.
void RaytracingAccelerationStructureManager::AddBottomLevelAS(
    D3D12RHI* d3dRHI,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags,
    BottomLevelAccelerationStructureGeometry& bottomLevelASGeometry,
    bool allowUpdate,
    bool performUpdateOnBuild)
{
    ThrowIfFalse(m_vBottomLevelAS.find(bottomLevelASGeometry.GetName()) == m_vBottomLevelAS.end(),
        L"A bottom level acceleration structure with that name already exists.");

    auto& bottomLevelAS = m_vBottomLevelAS[bottomLevelASGeometry.GetName()];

    bottomLevelAS.Initialize(d3dRHI, buildFlags, bottomLevelASGeometry, allowUpdate, performUpdateOnBuild);

    m_ASmemoryFootprint += bottomLevelAS.RequiredResultDataSizeInBytes();
    m_scratchResourceSize = max(bottomLevelAS.RequiredScratchSize(), m_scratchResourceSize);

    m_vBottomLevelAS[bottomLevelAS.GetName()] = bottomLevelAS;
}

D3D12_RAYTRACING_INSTANCE_DESC& RaytracingAccelerationStructureManager::GetBottomLevelASInstance(UINT index)
{
    assert(index < m_instanceDescs.size() && "Instance index out of range");
    
    return m_mappedInstanceData[index];
}

// Adds an instance of a bottom-level Acceleration Structure.
// Requires a call InitializeTopLevelAS() call to be added to top-level AS.
UINT RaytracingAccelerationStructureManager::AddBottomLevelASInstance(
    const wstring& bottomLevelASname,
    UINT instanceContributionToHitGroupIndex,
    TTransform transform,
    BYTE instanceMask)
{
    // ThrowIfFalse(m_numBottomLevelASInstances < m_bottomLevelASInstanceDescs.NumElements(), L"Not enough instance desc buffer size.");

    auto it = m_vBottomLevelAS.find(bottomLevelASname);
    if (it == m_vBottomLevelAS.end())
    {
        assert(false && "Bottom level AS not found");
        return UINT_MAX;
    }
    
    auto& bottomLevelAS = it->second;
    
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.InstanceMask = instanceMask;
    instanceDesc.InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex != UINT_MAX ? 
        instanceContributionToHitGroupIndex : bottomLevelAS.GetInstanceContributionToHitGroupIndex();
    instanceDesc.AccelerationStructure = bottomLevelAS.GetResource()->GetGPUVirtualAddress();
    
    SetInstanceTransform(instanceDesc, transform);
    
    UINT instanceIndex = static_cast<UINT>(m_instanceDescs.size());
    m_instanceDescs.push_back(instanceDesc);
    
    return instanceIndex;
};

UINT RaytracingAccelerationStructureManager::GetMaxInstanceContributionToHitGroupIndex()
{
    UINT maxInstanceContributionToHitGroupIndex = 0;
    for (UINT i = 0; i < m_numBottomLevelASInstances; i++)
    {
        auto& instanceDesc = GetBottomLevelASInstance(i);
        maxInstanceContributionToHitGroupIndex = max(maxInstanceContributionToHitGroupIndex, instanceDesc.InstanceContributionToHitGroupIndex);
    }
    return maxInstanceContributionToHitGroupIndex;
};

// Initializes the top-level Acceleration Structure.
void RaytracingAccelerationStructureManager::InitializeTopLevelAS(
    D3D12RHI* d3dRHI,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, 
    bool allowUpdate, 
    bool performUpdateOnBuild,
    const wchar_t* resourceName)
{
    m_topLevelAS.Initialize(d3dRHI, GetNumberOfBottomLevelASInstances(), buildFlags, allowUpdate, performUpdateOnBuild, resourceName);

    m_ASmemoryFootprint += m_topLevelAS.RequiredResultDataSizeInBytes();
    m_scratchResourceSize = max(m_topLevelAS.RequiredScratchSize(), m_scratchResourceSize);

    constexpr UINT elementSize = 4; // 4字节
    UINT elementCount = static_cast<UINT>((m_scratchResourceSize + elementSize - 1) / elementSize); // 向上取整
    
    RWStructuredBufferRef scratchBuffer = d3dRHI->CreateRWStructuredBuffer(
        elementSize, 
        elementCount);

    m_accelerationStructureScratch = scratchBuffer ? scratchBuffer->GetResource()->D3DResource : nullptr;
    
    if (resourceName && m_accelerationStructureScratch)
    {
        m_accelerationStructureScratch->SetName(L"Acceleration structure scratch resource");
    }
}

// Builds all bottom-level and top-level Acceleration Structures.
void RaytracingAccelerationStructureManager::Build(
    D3D12RHI* d3dRHI,
    ID3D12GraphicsCommandList4* commandList, 
    ID3D12DescriptorHeap* descriptorHeap,
    UINT frameIndex,
    bool bForceBuild)
{
    // 确保GPU缓冲区已更新
    // 首先检查是否需要创建或重新创建缓冲区
    if (!m_bottomLevelASInstanceDescs || bForceBuild)
    {
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(m_numBottomLevelASInstances);
        for (UINT i = 0; i < m_numBottomLevelASInstances; i++)
        {
            // 从之前收集的实例描述符复制数据
            // 假设有一个方法可以获取之前设置的描述符
            instanceDescs[i] = GetBottomLevelASInstance(i);
        }
        
        m_bottomLevelASInstanceDescs = d3dRHI->CreateStructuredBuffer(
            instanceDescs.data(),
            sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
            m_numBottomLevelASInstances);
    }
    
    // 如果底层结构已经创建但需要更新数据
    else if (m_bottomLevelASInstanceDescs)
    {
        // 直接将我们的实例描述符数据复制到GPU缓冲区
        void* mappedData = m_bottomLevelASInstanceDescs->GetResource()->mappedBaseAddress;
        if (mappedData)
        {
            // 为每个实例描述符复制数据
            for (UINT i = 0; i < m_numBottomLevelASInstances; i++)
            {
                static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mappedData)[i] = 
                    GetBottomLevelASInstance(i);
            }
            
            m_bottomLevelASInstanceDescs->GetResource()->D3DResource->Unmap(0, nullptr);
        }
    }

    // Build all bottom-level AS.
    {
        for (auto& bottomLevelASpair : m_vBottomLevelAS)
        {
            auto& bottomLevelAS = bottomLevelASpair.second;
            if (bForceBuild || bottomLevelAS.IsDirty())
            {
                D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGpuAddress = 0;
                bottomLevelAS.Build(commandList, m_accelerationStructureScratch.Get(), descriptorHeap, baseGeometryTransformGpuAddress);

                // Since a single scratch resource is reused, put a barrier in-between each call.
                // PERFORMANCE tip: use separate scratch memory per BLAS build to allow a GPU driver to overlap build calls.
                commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAS.GetResource()));
            }
        }
    }
    
    // Build the top-level AS.
    {
        bool performUpdate = false; // Always rebuild top-level Acceleration Structure.
        
        // 获取实例描述符缓冲区的GPU虚拟地址
        D3D12_GPU_VIRTUAL_ADDRESS instanceDescs = m_bottomLevelASInstanceDescs->GetResource()->D3DResource->GetGPUVirtualAddress();
        
        m_topLevelAS.Build(commandList, GetNumberOfBottomLevelASInstances(), instanceDescs, m_accelerationStructureScratch.Get(), descriptorHeap, performUpdate);

        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_topLevelAS.GetResource()));
    }
}