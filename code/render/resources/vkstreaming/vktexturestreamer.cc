//------------------------------------------------------------------------------
// vktexturestreamer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vktexturestreamer.h"

using namespace CoreGraphics;
using namespace IO;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkTextureStreamer, 'VKTS', Resources::StreamResourceLoader);
//------------------------------------------------------------------------------
/**
*/
VkTextureStreamer::VkTextureStreamer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkTextureStreamer::~VkTextureStreamer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTextureStreamer::SetupResourceFromStream(const Ptr<IO::Stream>& stream)
{
	bool result = false;
	const Ptr<Texture>& tex = this->resource.downcast<Texture>();
	if (tex->GetType() == Texture::Texture2D)
	{
		result = this->SetupTexture2DFromStream(stream);
		if (this->reuseTexture.isvalid())
		{
			this->reuseTexture->SetState(Texture::Initial);
			this->reuseTexture->Unlock();
		}
		tex->Unlock();
	}
	else if (tex->GetType() == Texture::TextureCube)
	{
		result = this->SetupTextureCubeFromStream(stream);
	}
	return result;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTextureStreamer::Reset()
{
	if (this->reuseTexture.isvalid())
	{
		this->reuseTexture->Unlock();
		this->reuseTexture = 0;
	}
	if (this->resource.isvalid())
	{
		this->resource->Unlock();
	}
	ResourceLoader::Reset();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTextureStreamer::OnLoadRequested()
{
	if (this->reuseTexture.isvalid())
	{
#ifdef NEBULA3_RESOURCE_DEBUG
		n_assert2(tex->GetNumMipLevels() != this->reuseTexture->GetNumMipLevels(),
			"Error: number of mips of target texture '%s' equals number of mips of source-texture '%s'."
			"if copying shall be enabled remove this error",
			tex->GetResourceId().AsString().AsCharPtr(), this->reuseTexture->GetResourceId().AsString().AsCharPtr());
#endif
		const Ptr<Texture>& tex = this->resource.downcast<Texture>();
		if (tex->GetNumMipLevels() < this->reuseTexture->GetNumMipLevels())
		{
			if (this->ReuseMips())
			{
				this->SetState(Resource::Loaded);
				return true;
			}
			else
			{
				this->SetState(Resource::Failed);
				return false;
			}
		}
	}

	return StreamResourceLoader::OnLoadRequested();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTextureStreamer::SetupTexture2DFromStream(const Ptr<IO::Stream>& stream)
{
	const Ptr<Texture>& tex = this->resource.downcast<Texture>();
	if (this->reuseTexture.isvalid())
	{
		n_assert2(tex->GetNumMipLevels() > this->reuseTexture->GetNumMipLevels(),
			"SetupTexture2DFromStream called but destination texture could be totally generated by"
			" copying texture in memory! call OGL4TextureStreamer::ReuseMips() instead so no "
			"unneeded stream is created!");
	}
#if ENABLE_LOAD_TIMERS
	_setup_timer(LoadingTimerTotal);
	_setup_timer(LoadingTimer1);
	_setup_timer(LoadingTimer2);
	_setup_timer(LoadingTimer3);
	_setup_timer(LoadingTimerCopy);

	_start_timer(LoadingTimerTotal);
#endif
	n_printf("loading resource '%s'\n", stream->GetURI().AsString().AsCharPtr());

	// --- load ---
	stream->SetAccessMode(Stream::ReadAccess);
	bool streamIsOpen = stream->Open();
	n_assert(streamIsOpen);
	void* srcData = stream->Map();

	// --- decompress ---
	// --- process ---
	// --- lock ---

	// determine how much mips we may reuse from another texture in memory
	// as this method is only called if no reuseTexture is set or a reuseTexture is set and has a smaller mip count
	// we can totally reuse the reuseTexture and not just partially
	// mipsToLoad refers to mips we want to load from file starting after all skippedMips and ending at all reusable mips
	SizeT mipsToLoad = tex->GetNumMipLevels();
	if (this->reuseTexture.isvalid())
	{
		mipsToLoad -= this->reuseTexture->GetNumMipLevels();
	}

	IndexT mipIdx;

	// width and height of current mipLevel
	uint curWidth = tex->GetWidth();
	uint curHeight = tex->GetHeight();

	// as we need to start with height and width of file' first mip map to skip mips in file
	// we need to multiply by 2 for each skipped mip
	// @todo: check if this can be done more efficiently using n_pow()
	for (mipIdx = 0; mipIdx < tex->GetSkippedMips(); mipIdx++)
	{
		curHeight = curHeight << 1;
		curWidth = curWidth << 1;
	}

	//uint numBytes = 0, rowBytes = 0, numRows = 0;
	uint numRows = 0;
	//uint rowIdx;
	int pitchSize = 0;

	// skip X number of mip levels
	uint bytesToSkip = 0, surfaceBytes = 0;
	for (mipIdx = 0; mipIdx < tex->GetSkippedMips(); mipIdx++)
	{
		this->GetSurfaceInfo(curWidth, curHeight, tex->GetPixelFormat(), &surfaceBytes, 0, 0);
		bytesToSkip += surfaceBytes;
		curWidth = curWidth >> 1;
		curHeight = curHeight >> 1;
	}

	// pointer to start of data (start + skip header infos + skipped mips)
	ubyte* texData = (ubyte*)srcData + sizeof(uint) + 124 + bytesToSkip;

	for (mipIdx = 0; mipIdx < mipsToLoad; mipIdx++)
	{
		int32_t width, height, depth;
		tex->MipDimensions(mipIdx, 0, width, height, depth);
		int32_t textureSize = width * height * PixelFormat::ToSize(tex->GetPixelFormat());
		Texture::MapInfo info;
		tex->Map(mipIdx, Texture::MapWrite, info);
		n_assert(info.rowPitch * info.depthPitch <= textureSize);
		memcpy(info.data, texData, textureSize);
		tex->Unmap(mipIdx);

		texData += textureSize;
		curWidth = curWidth >> 1;
		if (0 == curWidth)
		{
			curWidth = 1;
		}
		curHeight = curHeight >> 1;
		if (0 == curHeight)
		{
			curHeight = 1;
		}
	}

	if (this->reuseTexture.isvalid())
	{
		this->ReuseMips();
	}

#if ENABLE_LOAD_TIMERS
	_stop_timer(LoadingTimerTotal);
	_reset_accum_timer(LoadingTimer2);
	_reset_accum_timer(LoadingTimer3);
	_reset_accum_timer(LoadingTimerCopy);
	n_printf("total time needed for loading: %f\n", LoadingTimerTotal->GetSample());
	n_printf("time for copying total: %f\n", LoadingTimer1->GetSample());
	n_printf("time for MemCopy: %f\n", LoadingTimerCopy->GetSample());
	n_printf("time for timer2: %f\n", LoadingTimer2->GetSample());
	n_printf("time for timer3: %f\n", LoadingTimer3->GetSample());

	_discard_timer(LoadingTimerTotal);
	_discard_timer(LoadingTimer1);
	_discard_timer(LoadingTimer2);
	_discard_timer(LoadingTimer3);
	_discard_timer(LoadingTimerCopy);
#endif
	return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTextureStreamer::SetupTextureCubeFromStream(const Ptr<IO::Stream>& stream)
{
	const Ptr<Texture>& tex = this->resource.downcast<Texture>();
	if (this->reuseTexture.isvalid())
	{
		n_printf("NOTE: Reuse of CubeTextures is not supported yet");
		//n_assert2(tex->GetNumMipLevels() > this->reuseTexture->GetNumMipLevels(), 
		//    "SetupTextureCubeFromStream called but destination texture could be totally generated by"
		//    " copying texture in memory! call OGL4TextureStreamer::ReuseMips() instead so no "
		//    "unneeded stream is created!");
	}
	n_printf("loading resource '%s'\n", stream->GetURI().AsString().AsCharPtr());

	// --- load ---
	stream->SetAccessMode(Stream::ReadAccess);
	n_assert(stream->Open());
	void* srcData = stream->Map();

	// --- decompress ---
	// --- process ---
	// --- lock ---

	// determine how much mips we may reuse from another texture in memory
	// as this method is only called if no reuseTexture is set or a reuseTexture is set and has a smaller mip count
	// we can totally reuse the reuseTexture and not just partially
	// mipsToLoad refers to mips we want to load from file starting after all skippedMips and ending at all reusable mips
	SizeT mipsToLoad = tex->GetNumMipLevels();
	//if (this->reuseTexture.isvalid())
	//{
	//    mipsToLoad -= this->reuseTexture->GetNumMipLevels();
	//}

	// --- copy ---
	// width and height of current mipLevel
	uint curWidth = tex->GetWidth();
	uint curHeight = tex->GetHeight();

	// as we need to start with height and width of file' first mip map to skip mips in file
	// we need to multiply by 2 for each skipped mip (this value is per face!)
	// @todo: check if this can be done more efficiently using n_pow()
	IndexT mipIdx, faceIdx;
	for (mipIdx = 0; mipIdx < tex->GetSkippedMips(); mipIdx++)
	{
		curHeight = curHeight << 1;
		curWidth = curWidth << 1;
	}

	uint numRows = 0;
	int pitchSize = 0;

	// skip X number of mip levels
	uint bytesToSkip = 0, surfaceBytes = 0;
	// compute bytes to skip per face (skipped mips)
	for (mipIdx = 0; mipIdx < tex->GetSkippedMips(); mipIdx++)
	{
		this->GetSurfaceInfo(curWidth, curHeight, tex->GetPixelFormat(), &surfaceBytes, 0, 0);
		bytesToSkip += surfaceBytes;
		curWidth = curWidth >> 1;
		curHeight = curHeight >> 1;
	}

	// compute bytes of a whole face (to skip to next face)
	// therefore use previously computed bytes to skip and add non-skipped mips
	uint faceBytes = bytesToSkip;
	// save width and height of highest mip map we want to load from file
	uint initialHeight = curHeight;
	uint initialWidth = curWidth;

	for (mipIdx = 0; mipIdx < tex->GetNumMipLevels(); mipIdx++)
	{
		this->GetSurfaceInfo(curWidth, curHeight, tex->GetPixelFormat(), &faceBytes, 0, 0);
		faceBytes += surfaceBytes;
		curWidth = curWidth >> 1;
		curHeight = curHeight >> 1;
	}

	// copy mipMaps of each face
	// pointer to start of data (skip header infos)
	ubyte* startData = (ubyte*)srcData + sizeof(uint) + 124;

	for (faceIdx = 0; faceIdx < 6; faceIdx++)
	{
		ubyte* texData = startData + bytesToSkip + faceBytes * faceIdx;
		// reset height and width for new face
		curHeight = initialHeight;
		for (mipIdx = 0; mipIdx < mipsToLoad; mipIdx++)
		{
			GLint width, height, depth;
			tex->MipDimensions(mipIdx, faceIdx, width, height, depth);
			GLint textureSize = width * height * PixelFormat::ToSize(tex->GetPixelFormat());
			Texture::MapInfo info;
			tex->MapCubeFace((Texture::CubeFace)faceIdx, mipIdx, Texture::MapWrite, info);
			n_assert(info.rowPitch * info.depthPitch <= textureSize);
			memcpy(info.data, texData, textureSize);
			tex->UnmapCubeFace((Texture::CubeFace)faceIdx, mipIdx);

			texData += textureSize;
			curHeight = curHeight >> 1;
			if (0 == curHeight)
			{
				curHeight = 1;
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTextureStreamer::ReuseMips()
{
	n_assert(this->reuseTexture.isvalid());
	Ptr<Texture> tex = this->resource.downcast<Texture>();
	n_assert2(tex->GetType() == Texture::Texture2D, "Error: cube or volume textures are currently not supported for mip map reusage.");
	n_printf("reusing mips of texture '%s' (%i to %i)\n", tex->GetResourceId().Value(), this->reuseTexture->GetNumMipLevels(), tex->GetNumMipLevels());

	// copy
	IndexT srcMipIdx = 0;
	IndexT dstMipIdx = 0;
	if (this->reuseTexture->GetNumMipLevels() > tex->GetNumMipLevels())
	{
		srcMipIdx = this->reuseTexture->GetNumMipLevels() - tex->GetNumMipLevels();
	}
	else
	{
		dstMipIdx = tex->GetNumMipLevels() - this->reuseTexture->GetNumMipLevels();
	}
	while (dstMipIdx < tex->GetNumMipLevels())
	{
		n_assert(srcMipIdx < this->reuseTexture->GetNumMipLevels());
		int32_t width, height, depth;
		tex->MipDimensions(dstMipIdx, 0, width, height, depth);
		uint32_t textureSize = width * height * PixelFormat::ToSize(tex->GetPixelFormat());
		uint8_t* data = n_new_array(uint8_t, textureSize);
		VkTexture::Copy(this->reuseTexture, tex, width, height, depth, srcMipIdx, 0, 0, 0, 0, dstMipIdx, 0, 0, 0, 0);

		dstMipIdx++;
		srcMipIdx++;
	}

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTextureStreamer::GetSurfaceInfo(uint width, uint height, CoreGraphics::PixelFormat::Code fmt, uint* pNumBytes, uint* pRowBytes, uint* pNumRows) const
{
	uint numBytes = 0;
	uint rowBytes = 0;
	uint numRows = 0;

	// From the DXSDK docs:
	//
	//     When computing DXTn compressed sizes for non-square textures, the 
	//     following formula should be used at each mipmap level:
	//
	//         max(1, width � 4) x max(1, height � 4) x 8(DXT1) or 16(DXT2-5)
	//
	//     The pitch for DXTn formats is different from what was returned in 
	//     Microsoft DirectX 7.0. It now refers the pitch of a row of blocks. 
	//     For example, if you have a width of 16, then you will have a pitch 
	//     of four blocks (4*8 for DXT1, 4*16 for DXT2-5.)"

	if (fmt == PixelFormat::DXT1 ||
		fmt == PixelFormat::DXT1A ||
		fmt == PixelFormat::DXT1sRGB ||
		fmt == PixelFormat::DXT1AsRGB ||
		fmt == PixelFormat::DXT3 || 
		fmt == PixelFormat::DXT3sRGB ||
		fmt == PixelFormat::DXT5 ||
		fmt == PixelFormat::DXT5sRGB)
	{
		// Note: we support width and/or height being 0 in order to compute
		// offsets in functions like CBufferLockEntry::CopyBLEToPerfectSizedBuffer().
		int numBlocksWide = 0;
		if (width > 0)
			numBlocksWide = Math::n_max(1, width / 4);
		int numBlocksHigh = 0;
		if (height > 0)
			numBlocksHigh = Math::n_max(1, height / 4);
		//int numBlocks = numBlocksWide * numBlocksHigh;
		int numBytesPerBlock = (fmt == PixelFormat::DXT1 ? 8 : 16);
		rowBytes = numBlocksWide * numBytesPerBlock;
		numRows = numBlocksHigh;
	}
	else
	{
		uint bpp = PixelFormat::ToSize(fmt);
		rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
		numRows = height;
	}
	numBytes = rowBytes * numRows;
	if (pNumBytes != NULL)
		*pNumBytes = numBytes;
	if (pRowBytes != NULL)
		*pRowBytes = rowBytes;
	if (pNumRows != NULL)
		*pNumRows = numRows;
}


} // namespace Vulkan