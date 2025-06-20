// Modified version of DirectXTK12's source file

//--------------------------------------------------------------------------------------
// File: SpriteFont.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------


#pragma once

#include <stdint.h>
#include <windef.h>
#include "../Texture/Texture.h"

class SpriteFont
{
public:
	SpriteFont(ID3D12Device5* device, _In_z_ wchar_t const* fileName);

	// Describes a single character glyph.
	struct Glyph
	{
		uint32_t character;
		RECT subRect;
		float xOffset;
		float yOffset;
		float xAdvance;
	};

	Glyph const* FindGlyph(wchar_t character) const;

	uint32_t textureWidth;
	uint32_t textureHeight;
	DXGI_FORMAT textureFormat;
	uint32_t textureStride;
	uint32_t textureRows;
	std::vector<uint8_t> textureData;

	std::shared_ptr<Texture2D> GetFontTexture() { return fontTexture; }

private:
	void SetDefaultCharacter(wchar_t character);

private:
	std::vector<Glyph> glyphs;
	std::vector<uint32_t> glyphsIndex;
	Glyph const* defaultGlyph;
	float lineSpacing;

	std::shared_ptr<Texture2D> fontTexture;
};