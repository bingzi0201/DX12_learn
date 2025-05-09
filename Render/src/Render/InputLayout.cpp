#include "InputLayout.h"

void InputLayoutManager::AddInputLayout(const std::string& name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout)
{
	inputLayoutMap.insert({ name, inputLayout });
}

void InputLayoutManager::GetInputLayout(const std::string name, std::vector<D3D12_INPUT_ELEMENT_DESC>& outInputLayout) const
{
	auto Iter = inputLayoutMap.find(name);
	if (Iter == inputLayoutMap.end())
	{
		assert(0); //TODO
	}
	else
	{
		outInputLayout = Iter->second;
	}
}