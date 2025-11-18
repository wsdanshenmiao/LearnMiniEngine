#include "FourierTransform/FFT.h"
#include "Graphics/ShaderCompiler.h"
#include "Renderer.h"

namespace DSM{
	
	class DCT
    {
    public:
        DCT(bool enabledDebug = true) : m_FFT(enabledDebug) {}

        void Initialize(uint32_t width, uint32_t height)
        {
            m_FFT.Initialize(width, height);

            m_DCTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
            m_DCTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
            m_IDCTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
            m_IDCTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
            m_TmpUAV = g_Renderer.m_TextureHeap.Allocate();
            m_TmpSRV = g_Renderer.m_TextureHeap.Allocate();

            m_DCTRootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            m_DCTRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            m_DCTRootSig.Finalize(L"DCTRootSig");

            auto createPSO = [this](auto& pso, const auto& enterPoint, const auto& defines) {
                pso.SetRootSignature(m_DCTRootSig);
                auto desc = ShaderDesc{
                    .m_Type = ShaderType::Compute,
                    .m_Mode = ShaderMode::SM_6_6,
                    .m_FileName = "Shaders/DigitalImageProcessing/DCT.hlsl",
                    .m_EnterPoint = enterPoint
                };
                for(const auto& define : defines){
                    desc.m_Defines.AddDefine(define.first, define.second);
                }
                ShaderByteCode cs{desc};
                pso.SetComputeShader(cs);
                pso.Finalize();
            };

            std::vector<std::pair<std::string, std::string>> defines{};
            createPSO(m_DCTPSO, "DCTCS", defines);
            defines.emplace_back("IS_INVERSE", "1");
            createPSO(m_IDCTPSO, "DCTCS", defines);
            defines.clear();
            createPSO(m_HorizDCTPSO, "OriginPointDCTCS", defines);
            defines.emplace_back("IS_VERTICAL", "1");
            createPSO(m_VerticDCTPSO, "OriginPointDCTCS", defines);

            Resize(width, height);
        }

        void ExecuteDCT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV)
        {
            uint32_t width = inputTex.GetWidth();
            uint32_t height = inputTex.GetHeight();
            if(width != m_DCTOutputTex.GetWidth() ||
                height != m_DCTOutputTex.GetHeight()){
                Resize(width, height);
            }

            cmdList.CopyTextureRegion(m_TmpTex, 0, 0, 0, inputTex, RECT{0, 0, (LONG)width, (LONG)height});
            m_FFT.ExecuteFFT(cmdList, m_TmpTex, m_TmpSRV);

            cmdList.SetRootSignature(m_DCTRootSig);

            // 计算原点处的
            cmdList.SetPipelineState(m_HorizDCTPSO);
            cmdList.SetDescriptorTable(0, inputSRV);
            cmdList.SetDescriptorTable(1, m_DCTOutputUAV);
            cmdList.Dispatch1D(width, sm_ThreadSize * sm_ThreadSize);
            cmdList.InsertUAVBarrier(m_DCTOutputTex);

            cmdList.SetPipelineState(m_VerticDCTPSO);
            cmdList.SetDescriptorTable(0, inputSRV);
            cmdList.SetDescriptorTable(1, m_DCTOutputUAV);
            cmdList.Dispatch1D(height, sm_ThreadSize * sm_ThreadSize);
            cmdList.InsertUAVBarrier(m_DCTOutputTex);

            cmdList.SetPipelineState(m_DCTPSO);
            cmdList.SetDescriptorTable(0, m_FFT.GetFFTOutputSRV());
            cmdList.SetDescriptorTable(1, m_DCTOutputUAV);
            cmdList.Dispatch2D(width, height, sm_ThreadSize, sm_ThreadSize);
            cmdList.ExecuteCommandList();
        }

        void ExecuteIDCT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV)
        {
            uint32_t width = inputTex.GetWidth();
            uint32_t height = inputTex.GetHeight();
            if(width != m_IDCTOutputTex.GetWidth() ||
                height != m_IDCTOutputTex.GetHeight()){
                Resize(width, height);
            }

            cmdList.CopyTextureRegion(m_TmpTex, 0, 0, 0, inputTex, RECT{0, 0, (LONG)width, (LONG)height});
            m_FFT.ExecuteIFFT(cmdList, m_TmpTex, m_TmpSRV);

            cmdList.SetRootSignature(m_DCTRootSig);
            cmdList.SetPipelineState(m_IDCTPSO);
            cmdList.SetDescriptorTable(0, m_FFT.GetIFFTOutputSRV());
            cmdList.SetDescriptorTable(1, m_IDCTOutputUAV);
            cmdList.Dispatch2D(width, height, sm_ThreadSize, sm_ThreadSize);
            cmdList.ExecuteCommandList();
        }

        void Resize(uint32_t width, uint32_t height)
        {
            uint32_t doubledWidth = width * 2;
            uint32_t doubledHeight = height * 2;

            m_FFT.Resize(doubledWidth, doubledHeight);

            TextureDesc texDesc{};
            texDesc.m_Width = doubledWidth;
            texDesc.m_Height = doubledHeight;
            texDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            texDesc.m_Format = DXGI_FORMAT_R32G32_FLOAT;
            texDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            std::vector<float> initData(doubledWidth * doubledHeight * 2, 0.0f);
            D3D12_SUBRESOURCE_DATA subData{};
            subData.pData = initData.data();
            subData.RowPitch = doubledWidth * sizeof(float) * 2;
            subData.SlicePitch = subData.RowPitch * doubledHeight;
            m_TmpTex.Create(L"TmpTex", texDesc, {&subData, 1});
            m_TmpTex.CreateUnorderedAccessView(m_TmpUAV);
            m_TmpTex.CreateShaderResourceView(m_TmpSRV);

            texDesc.m_Width = width;
            texDesc.m_Height = height;
            m_DCTOutputTex.Create(L"DCTOutputTex", texDesc);
            m_DCTOutputTex.CreateUnorderedAccessView(m_DCTOutputUAV);
            m_DCTOutputTex.CreateShaderResourceView(m_DCTOutputSRV);
            m_IDCTOutputTex.Create(L"IDCTOutputTex", texDesc);
            m_IDCTOutputTex.CreateUnorderedAccessView(m_IDCTOutputUAV);
            m_IDCTOutputTex.CreateShaderResourceView(m_IDCTOutputSRV);
        }

        Texture& GetDCTOutputTex() noexcept { return m_DCTOutputTex; }
        const Texture& GetDCTOutputTex() const noexcept { return m_DCTOutputTex; }
        DescriptorHandle GetDCTOutputSRV() const noexcept { return m_DCTOutputSRV; }
        Texture& GetIDCTOutputTex() noexcept { return m_IDCTOutputTex; }
        const Texture& GetIDCTOutputTex() const noexcept { return m_IDCTOutputTex; }
        DescriptorHandle GetIDCTOutputSRV() const noexcept { return m_IDCTOutputSRV; }


    private:
        constexpr static uint32_t sm_ThreadSize = 16;

        Texture m_DCTOutputTex;
        DescriptorHandle m_DCTOutputUAV;
        DescriptorHandle m_DCTOutputSRV;
        Texture m_IDCTOutputTex;
        DescriptorHandle m_IDCTOutputUAV;
        DescriptorHandle m_IDCTOutputSRV;

        Texture m_TmpTex;
        DescriptorHandle m_TmpUAV;
        DescriptorHandle m_TmpSRV;

        RootSignature m_DCTRootSig{2, 0};

        ComputePSO m_DCTPSO;
        ComputePSO m_IDCTPSO;
        ComputePSO m_HorizDCTPSO;
        ComputePSO m_VerticDCTPSO;

        FFT m_FFT;
    };
}
