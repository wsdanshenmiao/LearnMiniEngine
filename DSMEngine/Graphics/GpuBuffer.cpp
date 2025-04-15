#include "GpuBuffer.h"
#include "LinearBufferAllocator.h"
#include "RenderContext.h"

/*namespace DSM{

    static std::map<LinearBufferDesc, LinearBufferAllocator> s_BufferAllocator{};
    
    GpuBuffer::GpuBuffer(const GpuBufferDesc& bufferDesc, void* initData)
        :m_BufferDesc(bufferDesc){
        // 从资源池中分配资源
        LinearBufferDesc allocDesc{};
        
        auto alignment = 0;
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::ConstantBuffer)) {
            alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }
        
        allocDesc.m_Flags = D3D12_RESOURCE_FLAG_NONE;
        allocDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        auto transitioningState = D3D12_RESOURCE_STATE_COMMON;
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::AccelStruct)) {
            transitioningState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        }
        if (bufferDesc.m_Usage == DSMResourceUsage::Upload) {
            transitioningState = D3D12_RESOURCE_STATE_GENERIC_READ;
            allocDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if (bufferDesc.m_Usage == DSMResourceUsage::Readback) {
            allocDesc.m_Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            transitioningState = D3D12_RESOURCE_STATE_COPY_DEST;
            allocDesc.m_HeapType = D3D12_HEAP_TYPE_READBACK;
        }
        
        if (Utility::HasAllFlags(bufferDesc.m_BindFlag, DSMBindFlag::UnorderedAccess)) {
            allocDesc.m_Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        if (!Utility::HasAllFlags(bufferDesc.m_BindFlag, DSMBindFlag::ShaderResource)) {
            allocDesc.m_Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::AccelStruct)) {
            allocDesc.m_Flags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
        }

        GpuResourceLocatioin resourceLoc{};
        if (auto it = s_BufferAllocator.find(allocDesc); it != s_BufferAllocator.end()) {
            resourceLoc = it->second.CreateBuffer(bufferDesc.m_Size, alignment);
        }
        else {
            s_BufferAllocator[allocDesc].Create(allocDesc);
            resourceLoc = s_BufferAllocator[allocDesc].CreateBuffer(bufferDesc.m_Size);
        }
        m_Resource = resourceLoc.m_Resource->GetResource();
        m_UsageState = resourceLoc.m_Resource->GetUsageState();
        m_GpuAddress = resourceLoc.m_GpuAddress;
        m_MappedAddress = resourceLoc.m_MappedAddress;

        // TODO: 封装完命令列表后转变资源状态，并将初始话资源拷贝到Buffer中
    }

    GpuBuffer::~GpuBuffer()
    {
        LinearBufferDesc allocDesc{};
        allocDesc.m_Flags = m_Resource->GetDesc().Flags;
        switch (m_BufferDesc.m_Usage) {
        case DSMResourceUsage::Default: allocDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT; break;
        case DSMResourceUsage::Upload: allocDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD; break;
        case DSMResourceUsage::Readback: allocDesc.m_HeapType = D3D12_HEAP_TYPE_READBACK; break;
        }
    }


    void GpuBuffer::Update(void* data, std::uint64_t size, std::uint64_t offset)
    {
        // 之后上传堆才可写出数据
        ASSERT(m_BufferDesc.m_Usage == DSMResourceUsage::Upload);

        if (m_MappedAddress == nullptr) {
            Map();
        }
        memcpy(GetMappedAddress<char>() + offset, data, size);
    }
}*/
