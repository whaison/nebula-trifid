#pragma once
//------------------------------------------------------------------------------
/**
	Utility functions to convert from Nebula enums to Vulkan values.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/imagefileformat.h"
#include "coregraphics/indextype.h"
#include "coregraphics/base/resourcebase.h"
namespace Vulkan
{
class VkTypes
{
public:

#pragma region Pixel stuff
	/// convert Nebula pixel format to vulkan pixel format
	static VkFormat AsVkFormat(CoreGraphics::PixelFormat::Code p);
	/// convert vulkan format back to nebula format
	static CoreGraphics::PixelFormat::Code AsNebulaPixelFormat(VkFormat f);
	/// convert pixel format to Vulkan component mapping
	static VkComponentMapping AsVkMapping(CoreGraphics::PixelFormat::Code p);
#pragma endregion

	/// convert primitive topology to vulkan
	static VkPrimitiveTopology AsVkPrimitiveType(CoreGraphics::PrimitiveTopology::Code t);
	/// convert the format to it's size
	static uint32_t AsByteSize(uint32_t semantic);
	/// convert vertex format to size
	static uint32_t AsVkSize(CoreGraphics::VertexComponent::Format f);
	/// convert vertex format to number of components
	static uint32_t AsVkNumComponents(CoreGraphics::VertexComponent::Format f);
	/// convert vertex component type to OGL4 symbolic type (single-element)
	static VkFormat AsVkVertexType(CoreGraphics::VertexComponent::Format f);

	/// convert vertex component semantic name as OGL4 declaration usage
	static GLenum AsOGL4VertexDeclarationUsage(CoreGraphics::VertexComponent::SemanticName n);
	/// convert antialias quality to D3D multisample type
	static GLuint AsOGL4MultiSampleType(CoreGraphics::AntiAliasQuality::Code c);
	/// convert Nebula3 access to OGL4 access
	static GLuint AsOGL4Access(Base::ResourceBase::Access access);
	/// convert Nebula3 usage to OGL4 usage
	static GLuint AsOGL4Usage(Base::ResourceBase::Usage usage, Base::ResourceBase::Access access);
	/// convert Nebula3 syncing to OGL4 syncing
	static GLuint AsOGL4Syncing(Base::ResourceBase::Syncing syncing);
	/// convert index type to DXGI format
	static GLenum IndexTypeAsOGL4Format(CoreGraphics::IndexType::Code indexType);
};
} // namespace Vulkan