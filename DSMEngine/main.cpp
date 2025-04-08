#include "Core/GameCore.h"
#include "Utilities/Utility.h"

using namespace DSM;

class Sandbox : public GameCore::IGameApp
{
public:
    virtual void Startup()override{}
    void Update(float deltaTime) override{};
    void RenderScene() override{};
    void Cleanup() override{};

private:
    
};

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    Sandbox sandbox{};
    return GameCore::RunApplication(sandbox, L"DSMEngine", hInstance, nShowCmd);
}