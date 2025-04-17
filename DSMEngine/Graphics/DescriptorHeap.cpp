#include "DescriptorHeap.h"
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
		return m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
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


	DescriptorHeap::DescriptorHeap(ID3D12Device* device)
		:m_Device(device) {
		ASSERT(device != nullptr);
	}

	
	//
	// D3D12DescriptorHeap Implementation
	//
	DescriptorHeap::~DescriptorHeap()
	{
		Destroy();
	}

	void DescriptorHeap::Create(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t maxCount)
	{
		if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
			m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		}
		else {
			m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}
		m_HeapDesc.NumDescriptors = maxCount;
		m_HeapDesc.Type = heapType;

		ASSERT_SUCCEEDED(m_Device->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
		m_DescriptorHeap->SetName(name.c_str());

		m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
		m_DescriptorSize = m_Device->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
		m_FirstHandle = { m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart() };
		m_NextFreeHandle = m_FirstHandle;
	}

	void DescriptorHeap::Destroy()
	{
		m_DescriptorHeap = nullptr;
	}

	void DescriptorHeap::Clear()
	{
		m_NextFreeHandle = m_FirstHandle;
		m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
	}

	bool DescriptorHeap::HasValidSpace(std::uint32_t numDexciptors) const noexcept
	{
		return m_NumFreeDescriptors >= numDexciptors;
	}

	bool DescriptorHeap::IsValidHandle(const DescriptorHandle& handle) const noexcept
	{
		auto cpuPtr = handle.GetCpuPtr();
		auto gpuPtr = handle.GetGpuPtr();
		if (cpuPtr < m_FirstHandle.GetCpuPtr() ||
			cpuPtr >= m_NextFreeHandle.GetCpuPtr() + m_HeapDesc.NumDescriptors * m_DescriptorSize) {
			return false;
		}
		if (gpuPtr - m_FirstHandle.GetGpuPtr() != cpuPtr - m_FirstHandle.GetCpuPtr()) {
			return false;
		}

		return true;
	}

	DescriptorHandle DescriptorHeap::AllocateAndCopy(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcHandle)
	{
		auto size = static_cast<UINT>(srcHandle.size());
		ASSERT(HasValidSpace(size));

		auto dstHandle = m_NextFreeHandle;
		m_Device->CopyDescriptors(1, &dstHandle, &size, size, srcHandle.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_NextFreeHandle += size * m_DescriptorSize;
		m_NumFreeDescriptors -= size;

		return dstHandle;
	}

	DescriptorHandle DescriptorHeap::Allocate(std::uint32_t count)
	{
		ASSERT(HasValidSpace(count));

		auto handle = m_NextFreeHandle;
		m_NextFreeHandle += m_DescriptorSize * count;
		m_NumFreeDescriptors -= count;

		return handle;
	}

	ID3D12DescriptorHeap* DescriptorHeap::GetHeap() const noexcept
	{
		return m_DescriptorHeap.Get();
	}

	std::uint32_t DescriptorHeap::GetOffsetOfHandle(const DescriptorHandle& handle) const noexcept
	{
		return (handle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize;
	}

	std::uint32_t DescriptorHeap::GetDescriptorSize() const noexcept
	{
		return m_DescriptorSize;
	}

	DescriptorHandle DescriptorHeap::operator[](std::uint32_t index) const noexcept
	{
		ASSERT(index < m_HeapDesc.NumDescriptors);

		return m_FirstHandle + index * m_DescriptorSize;
	}



	//
	// D3D12DescriptorCache Implementation
	//
	DescriptorCache::DescriptorCache(ID3D12Device* device, std::uint32_t maxCount)
		:m_Device(device) {
		ASSERT(device != nullptr);

		for (int i = 0; i < m_DescriptorHeaps.size(); ++i) {
			auto& heap = m_DescriptorHeaps[i];
			heap = std::make_unique<DescriptorHeap>(device);
			auto heapType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
			heap->Create(L"D3D12DescriptorCache" + Utility::UTF8ToWString(typeid(heapType).name()), heapType, maxCount);
		}
	}

	ID3D12DescriptorHeap* DescriptorCache::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const noexcept
	{
		auto index = static_cast<int>(heapType);
		ASSERT(index < m_DescriptorHeaps.size());
		return m_DescriptorHeaps[index]->GetHeap();
	}

	DescriptorHandle DescriptorCache::AllocateAndCopy(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcHandle)
	{
		auto index = static_cast<int>(heapType);
		ASSERT(index < m_DescriptorHeaps.size());
		return m_DescriptorHeaps[index]->AllocateAndCopy(srcHandle);
	}

	DescriptorHandle DescriptorCache::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t count)
	{
		auto index = static_cast<int>(heapType);
		ASSERT(index < m_DescriptorHeaps.size());
		return m_DescriptorHeaps[index]->Allocate(count);
	}

	void DescriptorCache::Clear()
	{
		for (auto& heap : m_DescriptorHeaps) {
			heap->Clear();
		}
	}
}
