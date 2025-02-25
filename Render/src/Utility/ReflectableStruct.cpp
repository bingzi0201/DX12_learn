#include "ReflectableStruct.h"
#include <string_view>

namespace rtti
{
	static thread_local Struct* curStruct = nullptr;
	size_t VarTypeData::GetSize() const 
	{
		return dimension * 4;
	}

	Struct::Struct()
	{
		curStruct = this;
	}

	VarTypeBase::VarTypeBase(VarTypeData&& varData)
	{
		offset = curStruct->structSize;
		if (!varData.semantic.empty())
		{
			uint rate = 1;
			uint semanticIndex = 0;
			// 初始化一个指针 ptr 指向字符串 varData.semantic 的最后一个字符
			// data() 方法返回一个指向字符串内容的 const char* 指针, size() 方法返回字符串的长度, size() - 1 是字符串最后一个字符的索引。
			char const* ptr = varData.semantic.data() + varData.semantic.size() - 1;
			while (ptr != (varData.semantic.data() - 1))
			{
				if (*ptr >= '0' && *ptr <= '9')
				{
					semanticIndex += rate * (int(*ptr) - int('0'));
					rate *= 10;
				}
				else
					break;
				--ptr;
			}
			auto leftSize = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(varData.semantic.data()) + 1;
			varData.semantic.resize(leftSize);
			varData.semanticIndex = semanticIndex;
		}
		curStruct->variables.emplace_back(std::move(varData));
		curStruct->structSize += varData.GetSize();
	}

	void Struct::GetMeshLayout(uint slot, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const
	{
		auto getFormat = [](VarTypeData const& d) -> DXGI_FORMAT
		{
			switch (d.scale)
			{
			case VarTypeData::ScaleType::Float:
				switch (d.dimension) {
				case 1:
					return DXGI_FORMAT_R32_FLOAT;
				case 2:
					return DXGI_FORMAT_R32G32_FLOAT;
				case 3:
					return DXGI_FORMAT_R32G32B32_FLOAT;
				case 4:
					return DXGI_FORMAT_R32G32B32A32_FLOAT;
				default:
					return DXGI_FORMAT_UNKNOWN;
				}
			case VarTypeData::ScaleType::UInt:
				switch (d.dimension) {
				case 1:
					return DXGI_FORMAT_R32_UINT;
				case 2:
					return DXGI_FORMAT_R32G32_UINT;
				case 3:
					return DXGI_FORMAT_R32G32B32_UINT;
				case 4:
					return DXGI_FORMAT_R32G32B32A32_UINT;
				default:
					return DXGI_FORMAT_UNKNOWN;
				}
			case VarTypeData::ScaleType::Int:
				switch (d.dimension) {
				case 1:
					return DXGI_FORMAT_R32_SINT;
				case 2:
					return DXGI_FORMAT_R32G32_SINT;
				case 3:
					return DXGI_FORMAT_R32G32B32_SINT;
				case 4:
					return DXGI_FORMAT_R32G32B32A32_SINT;
				default:
					return DXGI_FORMAT_UNKNOWN;
				}
			default:
				return DXGI_FORMAT_UNKNOWN;
			}
		};
		uint offset = 0;
		for (size_t i = 0; i < variables.size(); ++i)
		{
			auto& result = resultVector.emplace_back();   // 使用 emplace_back 在 resultVector 中构造一个新的 D3D12_INPUT_ELEMENT_DESC。
			auto& var = variables[i];
			result = {  var.semantic.c_str(),
						uint(var.semanticIndex),
						getFormat(var),
						slot,
						offset,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						uint(0) };
			offset += var.GetSize();
		}
	}


}