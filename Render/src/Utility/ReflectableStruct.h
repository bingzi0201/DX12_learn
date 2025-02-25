#pragma once
#include "../Common/stdafx.h"
#include <memory>
#include <DirectXMath.h>
#include <string>

namespace rtti
{
	// VarTypeData 描述变量元信息
	// 定义变量的类型（如 Float、Int、UInt）、维度（标量或向量）、语义（用于 DirectX 中的顶点属性描述）。
	// 提供基础信息供后续代码处理。
	struct VarTypeData
	{
		enum class ScaleType :vbyte
		{
			Float,
			Int,
			UInt
		};

		ScaleType scale;
		vbyte dimension;
		uint semanticIndex;
		std::string semantic;
		size_t GetSize() const;
	};

	class VarTypeBase;
	
	// Struct 用于操作结构化数据
	// 提供基于偏移量的结构成员访问机制，用于处理动态结构体数据。
	class Struct 
	{
	private:
		friend class VarTypeBase;
		std::vector<VarTypeData> variables;

	public:
		std::span<VarTypeData const> Variables() const { return variables; }
		size_t structSize = 0;
		Struct();
		Struct(Struct const&) = delete;
		Struct(Struct&&) = default;
		void GetMeshLayout(uint slot, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const;
	};

	// 作为变量类型的基类，提供对变量偏移量的管理及通用接口。
	class VarTypeBase
	{
	private:
		size_t offset;

	protected:
		VarTypeBase(VarTypeData&& varData);

	public:
		size_t Offset() const { return offset; }

	};

	// 为每种类型的变量提供类型安全的访问接口，同时继承偏移量管理功能。
	template<typename T>
	class VarType : public VarTypeBase
	{
	protected:
		VarType(VarTypeData&& varData) : VarTypeBase(std::move(varData)){}

	public:
		// T const&避免拷贝，只读
		T const& Get(void const* structPtr) const  // void const* 表示这是一个只读指针，不能通过它修改内存中的数据。
		{
			size_t ptrNum = reinterpret_cast<size_t>(structPtr);
			// reinterpret_cast<T const*>(ptrNum + Offset()) 是一个指向类型 T 的指针，指向内存地址 ptrNum + Offset()。'
			// * 对指针进行解引用，访问指针所指向的内存内容。
			return *reinterpret_cast<T const*>(ptrNum + Offset());
		}
		// T&避免拷贝，可修改
		T& Set(void* structPtr) const  // void* 表示这是一个可写指针，可以通过它修改内存中的数据。
		{
			size_t ptrNum = reinterpret_cast<size_t>(structPtr);
			return *reinterpret_cast<T*>(ptrNum + Offset());
		}
	};

	// 用于模板特化
	template<typename T>
	struct Var {};

	template<>
	struct Var<int32> : public VarType<int32> {
		Var(char const* semantic)
			: VarType<int32>(VarTypeData{
				VarTypeData::ScaleType::Int,    // 固定类型为 Int
				vbyte(1),                       // 固定维度为 1
				uint(0),                        // 默认语义索引为 0
				std::string(semantic)           // 用户输入的语义名称
				}) {}
	};
	template<>
	struct Var<uint> : public VarType<uint> {
		Var(char const* semantic) : VarType<uint>(VarTypeData{ VarTypeData::ScaleType::UInt, vbyte(1), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<float> : public VarType<float> {
		Var(char const* semantic) : VarType<float>(VarTypeData{ VarTypeData::ScaleType::Float, vbyte(1), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMFLOAT2> : public VarType<DirectX::XMFLOAT2> {
		Var(char const* semantic) : VarType<DirectX::XMFLOAT2>(VarTypeData{ VarTypeData::ScaleType::Float, vbyte(2), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMFLOAT3> : public VarType<DirectX::XMFLOAT3> {
		Var(char const* semantic) : VarType<DirectX::XMFLOAT3>(VarTypeData{ VarTypeData::ScaleType::Float, vbyte(3), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMFLOAT4> : public VarType<DirectX::XMFLOAT4> {
		Var(char const* semantic) : VarType<DirectX::XMFLOAT4>(VarTypeData{ VarTypeData::ScaleType::Float, vbyte(4), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMUINT2> : public VarType<DirectX::XMUINT2> {
		Var(char const* semantic) : VarType<DirectX::XMUINT2>(VarTypeData{ VarTypeData::ScaleType::UInt, vbyte(2), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMUINT3> : public VarType<DirectX::XMUINT3> {
		Var(char const* semantic) : VarType<DirectX::XMUINT3>(VarTypeData{ VarTypeData::ScaleType::UInt, vbyte(3), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMUINT4> : public VarType<DirectX::XMUINT4> {
		Var(char const* semantic) : VarType<DirectX::XMUINT4>(VarTypeData{ VarTypeData::ScaleType::UInt, vbyte(4), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMINT2> : public VarType<DirectX::XMINT2> {
		Var(char const* semantic) : VarType<DirectX::XMINT2>(VarTypeData{ VarTypeData::ScaleType::Int, vbyte(2), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMINT3> : public VarType<DirectX::XMINT3> {
		Var(char const* semantic) : VarType<DirectX::XMINT3>(VarTypeData{ VarTypeData::ScaleType::Int, vbyte(3), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct Var<DirectX::XMINT4> : public VarType<DirectX::XMINT4> {
		Var(char const* semantic) : VarType<DirectX::XMINT4>(VarTypeData{ VarTypeData::ScaleType::Int, vbyte(4), uint(0), std::string(semantic) }) {}
	};

}