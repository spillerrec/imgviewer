// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVulkanFunctions>
#include <string>
#include <stdexcept>

namespace vulkan
{

template<typename T, typename Parent, void (QVulkanDeviceFunctions::*Deleter)(typename Parent::ParentType, T, const VkAllocationCallbacks*)>
class VulkanHandle {
private:
	Parent parent = nullptr;
	T item = nullptr;
	
	void destroy() {
		if( item )
			(functions()->*Deleter)((typename Parent::ParentType)parent, item, nullptr);
	}
	
public:
	VulkanHandle(Parent p) : parent(p) { }
	~VulkanHandle() { destroy(); }
	
	VulkanHandle(const VulkanHandle<T, Parent, Deleter>&) = delete;
	VulkanHandle<T, Parent, Deleter>& operator=(const VulkanHandle<T, Parent, Deleter>&) = delete;
	
	VulkanHandle(VulkanHandle<T, Parent, Deleter>&& other) {
		parent = other.parent;
		item = other.item;
		other.item = nullptr;
	}
	
	VulkanHandle<T, Parent, Deleter> operator=(VulkanHandle<T, Parent, Deleter>&& rhs) {
		destroy();
		parent = rhs.parent;
		item = rhs.item;
		rhs.item = nullptr;
	}
	
	template<typename Constructor, typename Info>
	void create(Constructor constructor, Info info)
	{
		destroy();
		auto result = (functions()->*constructor)( parent, &info, nullptr, &item );
		if( result != VK_SUCCESS )
			throw std::logic_error( __PRETTY_FUNCTION__ + std::string(" failed with ") + std::to_string(result) );
	}
	
	void assign(T h)
	{
		destroy();
		item = h;
	}

	operator T() { return item; }
	Parent getParent() { return parent; }
	auto* functions() { return parent.functions(); }
};

}