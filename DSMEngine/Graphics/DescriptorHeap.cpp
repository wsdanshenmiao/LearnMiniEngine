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

	DescriptorHeap::DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType,std::uint32_t heapSize)
		:m_Allocator(heapSize){
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		}
		else {
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}
		heapDesc.Type = heapType;
		heapDesc.NumDescriptors = heapSize;

		auto pDevice = g_RenderContext.GetDevice();
		ASSERT_SUCCEEDED(pDevice->CreateDescriptorHeap(
			&heapDesc, IID_PPV_ARGS(m_DescriptorHeap.GetAddressOf())));
		m_DescriptorHeap->SetName(name.c_str());

		m_DescriptorSize = pDevice->GetDescriptorHandleIncrementSize(heapDesc.Type);
		m_FirstHandle = { m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart() };
	}

	void DescriptorHeap::Clear()
	{
		m_Allocator.Clear();
	}

	bool DescriptorHeap::HasValidSpace(std::uint32_t numDesciptors) const noexcept
	{
		return m_Allocator.UsedSize() + numDesciptors <= m_Allocator.MaxSize();;
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



	//
	// DescriptorAllocator Implementation
	//
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(std::uint32_t count)
	{
		if (m_CurrHeap == nullptr || !m_CurrHeap->HasValidSpace(count)) {
			auto newHeap = std::make_unique<DescriptorHeap>(
				L"DescriptorAllocator::DescriptorHeap", m_HeapType, sm_NumDescriptorsPerHeap);
			m_CurrHeap = newHeap.get();
			sm_DescriptorHeapPool.emplace_back(std::move(newHeap));
		}

		return m_CurrHeap->Allocate(count);
	}

}
