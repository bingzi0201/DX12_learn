#pragma once

#include "../Utils/D3D12Utils.h"
#include <unordered_map>

class InputLayoutManager
{
public:
	void AddInputLayout(const std::string& name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);
	void GetInputLayout(const std::string name, std::vector<D3D12_INPUT_ELEMENT_DESC>& outInputLayout) const;

private:
	std::unordered_map<std::string/*Name*/, std::vector<D3D12_INPUT_ELEMENT_DESC>> inputLayoutMap;
};
