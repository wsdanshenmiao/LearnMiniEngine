targetName = "LearnMiniEngine"
target(targetName)
    set_kind("static")
    set_targetdir(path.join(binDir, targetName))

    add_deps("Imgui")

    add_rules("ShaderCopy")
    
    add_includedirs("./",{public = true})
    add_files("**.cpp")
    add_headerfiles("**.h")
    add_headerfiles("Shaders/**.hlsli", "Shaders/**.hlsl")

    add_includedirs("./", {public = true})

target_end()