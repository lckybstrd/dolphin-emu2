// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <xxhash.h>
#include "Core/HW/Memmap.h"
#include "VideoBackends/D3D/D3DBase.h"
#include "VideoBackends/D3D/D3DState.h"
#include "VideoBackends/D3D/D3DUtil.h"
#include "VideoBackends/D3D/FramebufferManager.h"
#include "VideoBackends/D3D/GeometryShaderCache.h"
#include "VideoBackends/D3D/PixelShaderCache.h"
#include "VideoBackends/D3D/PSTextureEncoder.h"
#include "VideoBackends/D3D/TextureCache.h"
#include "VideoBackends/D3D/TextureEncoder.h"
#include "VideoBackends/D3D/VertexShaderCache.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/VideoConfig.h"

namespace DX11
{

static TextureEncoder* g_encoder = nullptr;
const size_t MAX_COPY_BUFFERS = 32;
ID3D11Buffer* efbcopycbuf[MAX_COPY_BUFFERS] = { 0 };

TextureCache::TCacheEntry::~TCacheEntry()
{
	texture->Release();
	int foo = 1;
	XXH32(&foo, sizeof(int), 0x34);
}

void TextureCache::TCacheEntry::Bind(unsigned int stage)
{
	D3D::stateman->SetTexture(stage, texture->GetSRV());
}

bool TextureCache::TCacheEntry::Save(const std::string& filename, unsigned int level)
{
	// TODO: Somehow implement this (D3DX11 doesn't support dumping individual LODs)
	static bool warn_once = true;
	if (level && warn_once)
	{
		WARN_LOG(VIDEO, "Dumping individual LOD not supported by D3D11 backend!");
		warn_once = false;
		return false;
	}

	ID3D11Texture2D* pNewTexture = nullptr;
	ID3D11Texture2D* pSurface = texture->GetTex();
	D3D11_TEXTURE2D_DESC desc;
	pSurface->GetDesc(&desc);

	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_STAGING;

	HRESULT hr = D3D::device->CreateTexture2D(&desc, nullptr, &pNewTexture);

	bool saved_png = false;

	if (SUCCEEDED(hr) && pNewTexture)
	{
		D3D::context->CopyResource(pNewTexture, pSurface);

		D3D11_MAPPED_SUBRESOURCE map;
		HRESULT hr = D3D::context->Map(pNewTexture, 0, D3D11_MAP_READ_WRITE, 0, &map);
		if (SUCCEEDED(hr))
		{
			saved_png = TextureToPng((u8*)map.pData, map.RowPitch, filename, desc.Width, desc.Height);
			D3D::context->Unmap(pNewTexture, 0);
		}
		SAFE_RELEASE(pNewTexture);
	}

	return saved_png;
}

void TextureCache::TCacheEntry::Load(unsigned int width, unsigned int height,
	unsigned int expanded_width, unsigned int level)
{
	D3D::ReplaceRGBATexture2D(texture->GetTex(), TextureCache::temp, width, height, expanded_width, level, usage);
}

TextureCache::TCacheEntryBase* TextureCache::CreateTexture(unsigned int width, unsigned int height,
	unsigned int tex_levels, PC_TexFormat pcfmt)
{
	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	D3D11_CPU_ACCESS_FLAG cpu_access = (D3D11_CPU_ACCESS_FLAG)0;

	if (tex_levels == 1)
	{
		usage = D3D11_USAGE_DYNAMIC;
		cpu_access = D3D11_CPU_ACCESS_WRITE;
	}

	const D3D11_TEXTURE2D_DESC texdesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM,
		width, height, 1, tex_levels, D3D11_BIND_SHADER_RESOURCE, usage, cpu_access);

	ID3D11Texture2D *pTexture;
	const HRESULT hr = D3D::device->CreateTexture2D(&texdesc, nullptr, &pTexture);
	CHECK(SUCCEEDED(hr), "Create texture of the TextureCache");

	TCacheEntryConfig config;
	config.width = width;
	config.height = height;
	config.levels = tex_levels;

	TCacheEntry* const entry = new TCacheEntry(config, new D3DTexture2D(pTexture, D3D11_BIND_SHADER_RESOURCE));
	entry->usage = usage;

	// TODO: better debug names
	D3D::SetDebugObjectName((ID3D11DeviceChild*)entry->texture->GetTex(), "a texture of the TextureCache");
	D3D::SetDebugObjectName((ID3D11DeviceChild*)entry->texture->GetSRV(), "shader resource view of a texture of the TextureCache");

	SAFE_RELEASE(pTexture);

	return entry;
}

void TextureCache::TCacheEntry::FromRenderTarget(u32 dstAddr, unsigned int dstFormat,
	PEControl::PixelFormat srcFormat, const EFBRectangle& srcRect,
	bool isIntensity, bool scaleByHalf, unsigned int cbufid,
	const float *colmat)
{
	if (type != TCET_EC_DYNAMIC || g_ActiveConfig.bCopyEFBToTexture)
	{
		g_renderer->ResetAPIState();

		// stretch picture with increased internal resolution
		const D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, (float)config.width, (float)config.height);
		D3D::context->RSSetViewports(1, &vp);

		// set transformation
		if (nullptr == efbcopycbuf[cbufid])
		{
			const D3D11_BUFFER_DESC cbdesc = CD3D11_BUFFER_DESC(28 * sizeof(float), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT);
			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = colmat;
			HRESULT hr = D3D::device->CreateBuffer(&cbdesc, &data, &efbcopycbuf[cbufid]);
			CHECK(SUCCEEDED(hr), "Create efb copy constant buffer %d", cbufid);
			D3D::SetDebugObjectName((ID3D11DeviceChild*)efbcopycbuf[cbufid], "a constant buffer used in TextureCache::CopyRenderTargetToTexture");
		}
		D3D::stateman->SetPixelConstants(efbcopycbuf[cbufid]);

		const TargetRectangle targetSource = g_renderer->ConvertEFBRectangle(srcRect);
		// TODO: try targetSource.asRECT();
		const D3D11_RECT sourcerect = CD3D11_RECT(targetSource.left, targetSource.top, targetSource.right, targetSource.bottom);

		// Use linear filtering if (bScaleByHalf), use point filtering otherwise
		if (scaleByHalf)
			D3D::SetLinearCopySampler();
		else
			D3D::SetPointCopySampler();

		// if texture is currently in use, it needs to be temporarily unset
		u32 textureSlotMask = D3D::stateman->UnsetTexture(texture->GetSRV());
		D3D::stateman->Apply();

		D3D::context->OMSetRenderTargets(1, &texture->GetRTV(), nullptr);

		// Create texture copy
		D3D::drawShadedTexQuad(
			(srcFormat == PEControl::Z24) ? FramebufferManager::GetEFBDepthTexture()->GetSRV() : FramebufferManager::GetEFBColorTexture()->GetSRV(),
			&sourcerect, Renderer::GetTargetWidth(), Renderer::GetTargetHeight(),
			(srcFormat == PEControl::Z24) ? PixelShaderCache::GetDepthMatrixProgram(true) : PixelShaderCache::GetColorMatrixProgram(true),
			VertexShaderCache::GetSimpleVertexShader(), VertexShaderCache::GetSimpleInputLayout(), GeometryShaderCache::GetCopyGeometryShader());

		D3D::context->OMSetRenderTargets(1, &FramebufferManager::GetEFBColorTexture()->GetRTV(), FramebufferManager::GetEFBDepthTexture()->GetDSV());

		g_renderer->RestoreAPIState();

		// Restore old texture in all previously used slots, if any
		D3D::stateman->SetTextureByMask(textureSlotMask, texture->GetSRV());
	}

	if (!g_ActiveConfig.bCopyEFBToTexture)
	{
		u8* dst = Memory::GetPointer(dstAddr);
		size_t encoded_size = g_encoder->Encode(dst, dstFormat, srcFormat, srcRect, isIntensity, scaleByHalf);

		u64 hash = GetHash64(dst, (int)encoded_size, g_ActiveConfig.iSafeTextureCache_ColorSamples);

		size_in_bytes = (u32)encoded_size;

		// Mark texture entries in destination address range dynamic unless caching is enabled and the texture entry is up to date
		if (!g_ActiveConfig.bEFBCopyCacheEnable)
			TextureCache::MakeRangeDynamic(addr, (u32)encoded_size);
		else if (!TextureCache::Find(addr, hash))
			TextureCache::MakeRangeDynamic(addr, (u32)encoded_size);

		this->hash = hash;
	}
}

TextureCache::TCacheEntryBase* TextureCache::CreateRenderTargetTexture(
	unsigned int scaled_tex_w, unsigned int scaled_tex_h, unsigned int layers)
{
	TCacheEntryConfig config;
	config.width = scaled_tex_w;
	config.height = scaled_tex_h;
	config.layers = layers;
	config.rendertarget = true;

	return new TCacheEntry(config, D3DTexture2D::Create(scaled_tex_w, scaled_tex_h,
		(D3D11_BIND_FLAG)((int)D3D11_BIND_RENDER_TARGET | (int)D3D11_BIND_SHADER_RESOURCE),
		D3D11_USAGE_DEFAULT, DXGI_FORMAT_R8G8B8A8_UNORM, 1, layers));
}

TextureCache::TextureCache()
{
	// FIXME: Is it safe here?
	g_encoder = new PSTextureEncoder;
	g_encoder->Init();
}

TextureCache::~TextureCache()
{
	for (unsigned int k = 0; k < MAX_COPY_BUFFERS; ++k)
		SAFE_RELEASE(efbcopycbuf[k]);

	g_encoder->Shutdown();
	delete g_encoder;
	g_encoder = nullptr;
}

}
