#include "DescriptorHeap.h"
#include "RenderContext.h"
#include "../Utilities/Macros.h"

namespace DSM {
	//
	// D3D12DescriptorHandle Implementation
	//
	DescriptorHandle::DescriptorHandle()
		:m_CPUHandle(D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN),
		m_GPUHandle(D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
	}

	DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
		:m_CPUHandle(cpuHandle), m_GPUHandle(gpuHandle) {
	}

	std::size_t DescriptorHandle::GetCpuPtr() const noexcept
	{
		return m_CPUHandle.ptr;
	}

	std::uint64_t DescriptorHandle::GetGpuPtr() const noexcept
	{
		return m_GPUHandle.ptr;
	}

	bool DescriptorHandle::IsValid() const noexcept
	{
		return m_CPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	bool DescriptorHandle::IsShaderVisible() const noexcept
	{
		return m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	DescriptorHandle DescriptorHandle::operator+(int offset) const noexcept
	{
		auto ret = *this;
		ret += offset;
		return ret;
	}

	void DescriptorHandle::operator+=(int offset) noexcept
	{
		if (m_CPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_CPUHandle.ptr += offset;
		}
		if (m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_GPUHandle.ptr += offset;
		}
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE* DescriptorHandle::operator&() const noexcept
	{
		return &m_CPUHandle;
	}

	DescriptorHandle::operator D3D12_CPU_DESCRIPTOR_HANDLE() const noexcept
	{
		return m_CPUHandle;
	}

	DescriptorHandle::operator D3D12_GPU_DESCRIPTOR_HANDLE() const noexcept
	{
		return m_GPUHandle;
	}

	DescriptorHeap::DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t heapSize)
		:m_Allocator(heapSize){
		D3D12_DESCRIPTOR_HEAP_FLAGS flags;
		if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
			flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		}
		else {
			flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}
		Create(name, heapType, heapSize, flags);
	}

	void DescriptorHeap::Clear()
	{
		m_Allocator.Clear();
	}

	bool DescriptorHeap::HasValidSpace(std::uint32_t numDesciptors) const noexcept
	{
		return (m_Allocator.UsedSize() + numDesciptors) <= m_Allocator.MaxSize();
	}

	bool DescriptorHeap::IsValidHandle(const DescriptorHandle& handle) const noexcept
	{
		auto cpuPtr = handle.GetCpuPtr();
		auto gpuPtr = handle.GetGpuPtr();
		if (cpuPtr < m_FirstHandle.GetCpuPtr() ||
			cpuPtr >= m_FirstHandle.GetCpuPtr() + m_Allocator.MaxSize() * m_DescriptorSize) {
			return false;
		}
		if (gpuPtr - m_FirstHandle.GetGpuPtr() != cpuPtr - m_FirstHandle.GetCpuPtr()) {
			return false;
		}

		return true;
	}

	DescriptorHandle DescriptorHeap::Allocate(std::uint32_t count)
	{
		ASSERT(HasValidSpace(count));
		auto offset = m_Allocator.Allocate(count);
		auto handle = m_FirstHandle + offset * m_DescriptorSize;
		return handle;
	}

	ID3D12DescriptorHeap* DescriptorHeap::GetHeap() const noexcept
	{
		return m_DescriptorHeap.Get();
	}

	std::uint32_t DescriptorHeap::GetOffsetOfHandle(const DescriptorHandle& handle) const noexcept
	{
		ASSERT(IsValidHandle(handle));
		return (handle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize;
	}

	std::uint32_t DescriptorHeap::GetDescriptorSize() const noexcept
	{
		return m_DescriptorSize;
	}

	DescriptorHandle DescriptorHeap::operator[](std::uint32_t index) const noexcept
	{
		ASSERT(index < m_Allocator.MaxSize());

		return m_FirstHandle + index * m_DescriptorSize;
	}

	void DescriptorHeap::Create(
		const std::wstring& name, 
		D3D12_DESCRIPTOR_HEAP_TYPE heapType, 
		std::uint32_t heapSize, 
		D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Type = heapType;
		heapDesc.NumDescriptors = heapSize;
		heapDesc.Flags = flags;

		auto pDevice = g_RenderContext.GetDevice();
		ASSERT_SUCCEEDED(pDevice->CreateDescriptorHeap(
			&heapDesc, IID_PPV_ARGS(m_DescriptorHeap.GetAddressOf())));
		m_DescriptorHeap->SetName(name.c_str());

		m_DescriptorSize = pDevice->GetDescriptorHandleIncrementSize(heapDesc.Type);
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = heapDesc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ?
			m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart() :
			D3D12_GPU_DESCRIPTOR_HANDLE{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
		m_FirstHandle = { m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			gpuHandle };
	}


	//
	// DescriptorAllocator Implementation
	//
	DescriptorAllocator::~DescriptorAllocator()
	{
		std::lock_guard lock{sm_Mutex};
		for (auto& page : m_FullPages) {
			sm_AvailablePages[m_HeapType].push(page);
		}
		if (m_CurrHeap != nullptr) {
			sm_AvailablePages[m_HeapType].push(m_CurrHeap);
		}
	}

	DescriptorHandle DescriptorAllocator::AllocateDescriptor(std::uint32_t count)
	{
		if (m_CurrHeap == nullptr || !m_CurrHeap->m_Heap.HasValidSpace(count)) {
			std::lock_guard lock{sm_Mutex};
			if (m_CurrHeap != nullptr) {
				m_CurrHeap->m_Heap.Clear();
				m_FullPages.push_back(m_CurrHeap);
			}
			
			if (sm_AvailablePages[m_HeapType].empty()) {
				DescriptorHeap newHeap{
					L"DescriptorAllocator::DescriptorHeap",
					m_HeapType,
					sm_NumDescriptorsPerHeap,
					D3D12_DESCRIPTOR_HEAP_FLAG_NONE };
				DescriptorPage* newPage = new DescriptorPage{ .m_Heap = std::move(newHeap), .m_UsedCount = 0 };
				m_CurrHeap = newPage;
				sm_DescriptorPagePool.emplace_back(newPage);
			}
			else {
				m_CurrHeap = sm_AvailablePages[m_HeapType].front();
				sm_AvailablePages[m_HeapType].pop();
			}
		}

		m_CurrHeap->m_UsedCount += count;
		return m_CurrHeap->m_Heap.Allocate(count);
	}

	void DescriptorAllocator::FreeDescriptor(const DescriptorHandle& handle, std::uint32_t count)
	{
		std::lock_guard lock{ sm_Mutex };

		if (m_CurrHeap != nullptr && m_CurrHeap->m_Heap.IsValidHandle(handle)) {
			m_CurrHeap->m_UsedCount -= count;
		}
		else {
			auto it = std::find_if(m_FullPages.begin(), m_FullPages.end(),[&handle](DescriptorPage* page) {
				return page->m_Heap.IsValidHandle(handle);
			});
			if (it != m_FullPages.end()) {
				(*it)->m_UsedCount -= count;
				if ((*it)->m_UsedCount <=0) {
					sm_AvailablePages[m_HeapType].push(*it);
					*it = std::move(m_FullPages.back());
					m_FullPages.pop_back();
				}
			}
		}
	}
}
