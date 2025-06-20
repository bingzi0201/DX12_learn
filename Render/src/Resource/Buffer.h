#pragma once

#include "Resource.h"
#include "View.h"

class Buffer
{
public:
	Buffer() {}
	virtual ~Buffer() {}

	Resource* GetResource() { return resourceLocation.underlyingResource; }

public:
	ResourceLocation resourceLocation;
};

class ConstantBuffer : public Buffer
{
};
typedef std::shared_ptr<ConstantBuffer> ConstantBufferRef;


class StructuredBuffer : public Buffer
{
public:
	ShaderResourceView* GetSRV()
	{
		return srv.get();
	}

	void SetSRV(std::unique_ptr<ShaderResourceView>& InSRV)
	{
		srv = std::move(InSRV);
	}

private:
	std::unique_ptr<ShaderResourceView> srv = nullptr;
};
typedef std::shared_ptr<StructuredBuffer> StructuredBufferRef;


class RWStructuredBuffer : public Buffer
{
public:
	ShaderResourceView* GetSRV()
	{
		return srv.get();
	}

	void SetSRV(std::unique_ptr<ShaderResourceView>& InSRV)
	{
		srv = std::move(InSRV);
	}

	UnorderedAccessView* GetUAV()
	{
		return uav.get();
	}

	void SetUAV(std::unique_ptr<UnorderedAccessView>& InUAV)
	{
		uav = std::move(InUAV);
	}

private:
	std::unique_ptr<ShaderResourceView> srv = nullptr;
	std::unique_ptr<UnorderedAccessView> uav = nullptr;
};
typedef std::shared_ptr<RWStructuredBuffer> RWStructuredBufferRef;


class AccelerationStructureBuffer : public Buffer
{
public:
    ShaderResourceView* GetSRV()
    {
        return srv.get();
    }

    void SetSRV(std::unique_ptr<ShaderResourceView>& srv)
    {
        srv = std::move(srv);
    }

private:
	std::unique_ptr<ShaderResourceView> srv = nullptr;
};
typedef std::shared_ptr<AccelerationStructureBuffer> ASBufferRef;


class VertexBuffer : public Buffer
{
};
typedef std::shared_ptr<VertexBuffer> VertexBufferRef;


class IndexBuffer : public Buffer
{
};
typedef std::shared_ptr<IndexBuffer> IndexBufferRef;


class ReadBackBuffer : public Buffer
{
};
typedef std::shared_ptr<ReadBackBuffer> ReadBackBufferRef;