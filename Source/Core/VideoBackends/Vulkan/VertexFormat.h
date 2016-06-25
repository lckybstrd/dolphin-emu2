// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "VideoCommon/NativeVertexFormat.h"
#include "VideoBackends/Vulkan/VulkanImports.h"

namespace Vulkan {

class VertexFormat : public ::NativeVertexFormat
{
public:
	VertexFormat(const PortableVertexDeclaration& in_vtx_decl);

	// Passed to pipeline state creation
	const VkPipelineVertexInputStateCreateInfo& GetVertexInputStateInfo() const { return m_input_state_info; }

	// Converting PortableVertexDeclaration -> Vulkan types
	void MapAttributes();
	void SetupInputState();

	// Not used in the Vulkan backend.
	void SetupVertexPointers() override;

private:
	void AddAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

	VkVertexInputBindingDescription m_binding_description = {};

	std::array<VkVertexInputAttributeDescription, MAX_VERTEX_ATTRIBUTES> m_attribute_descriptions = {};

	VkPipelineVertexInputStateCreateInfo m_input_state_info = {};

	uint32_t m_num_attributes = 0;
};

}
