#pragma once
#ifndef __FFT_H__
#define __FFT_H__

#include "Graphics/CommandList/ComputeCommandList.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"

namespace DSM{
    class FFT
    {
    public:
        FFT(bool enabledOutput = true) : 
            m_EnabledDebugOutput(enabledOutput), 
            m_FFTRootSig{enabledOutput ? 4u : 3u, 0} {}

        void Initialize(uint32_t width, uint32_t height);

        void ExecuteFFT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV);
        void ExecuteIFFT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV);

        void Resize(uint32_t width, uint32_t height);

        Texture& GetFFTOutputTex() noexcept { return m_FFTOutputTex; }
        const Texture& GetFFTOutputTex() const noexcept { return m_FFTOutputTex; }
        DescriptorHandle GetFFTOutputSRV() const noexcept { return m_FFTOutputSRV; }
        Texture& GetFFTDebugTex() noexcept { return m_FFTDebugTex; }
        const Texture& GetFFTDebugTex() const noexcept { return m_FFTDebugTex; }
        DescriptorHandle GetFFTDebugSRV() const noexcept { return m_FFTDebugSRV; }
        DescriptorHandle GetIFFTOutputSRV() const noexcept { return m_IFFTOutputSRV; }
        Texture& GetIFFTOutputTex() noexcept { return m_IFFTOutputTex; }
        const Texture& GetIFFTOutputTex() const noexcept { return m_IFFTOutputTex; }

    private:
        void CreateReverseIndicesBuffer(uint32_t width, uint32_t height);
        void Execute(ComputeCommandList& cmdList, bool inverse);

    private:
        struct FFTConstants
        {
            uint32_t numStages;
            uint32_t stage;
            float sign;
        };

        static constexpr uint32_t sm_ThreadSize = 256;

        bool m_EnabledDebugOutput = true;

        // 位反转后的索引
        GpuBuffer m_HorizReverseIndicesBuffer{};
        GpuBuffer m_VerticReverseIndicesBuffer{};

        Texture m_FFTOutputTex;
        DescriptorHandle m_FFTOutputUAV{};
        DescriptorHandle m_FFTOutputSRV{};

        Texture m_IFFTOutputTex;
        DescriptorHandle m_IFFTOutputUAV{};
        DescriptorHandle m_IFFTOutputSRV{};

        Texture m_FFTDebugTex;
        DescriptorHandle m_FFTDebugSRV{};
        DescriptorHandle m_FFTDebugUAV{};

        RootSignature m_FFTRootSig;

        ComputePSO m_FFTHorizGroupMemPSO;
        ComputePSO m_FFTVerticGroupMemPSO;
        ComputePSO m_FFTHorizPSO;
        ComputePSO m_FFTVerticPSO;
        
        ComputePSO m_FFTHorizBitReversedPSO;
        ComputePSO m_FFTVerticBitReversedPSO;

        ComputePSO m_IFFTScalePSO;
    };
}


#endif  // __FFT_H__