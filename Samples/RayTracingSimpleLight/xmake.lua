targetName = "RayTracingSimpleLight"
target(targetName)
    set_kind("binary")
    set_targetdir(path.join(binDir, targetName))

    add_deps("LearnMiniEngine")
    add_rules("Imguiini")
    add_rules("ShaderCopy")
    add_rules("EngineShaderCopy")
    add_rules("ModelCopy")
    add_rules("TextureCopy")

    add_files("**.cpp")
    add_headerfiles("**.h")
    add_headerfiles("Shaders/**.hlsli", "Shaders/**.hlsl")

target_end()