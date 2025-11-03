#pragma once
#ifndef __FFT_H__
#define __FFT_H__

#include "Graphics/CommandList/ComputeCommandList.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ShaderCompiler.h"

namespace DSM{
    class FFT
    {
    public:
        void Initialize(uint32_t width, uint32_t height)
        {

        }

        void ExecuteFFT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV)
        {

        }


    private:
        // 位反转后的索引
        GpuBuffer m_ReverseIndicesBuffer{};
        Texture m_OutputTex;

        RootSignature m_FFTRootSig;
        ComputePSO m_FFTPSO;
    };
}


#endif  // __FFT_H__