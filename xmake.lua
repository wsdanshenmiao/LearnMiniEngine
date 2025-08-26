set_project("LearnMiniEngine")

if is_os("windows") then 
    add_defines("UNICODE")
    add_defines("_UNICODE")
end

add_rules("mode.debug", "mode.release")
set_languages("c99", "cxx20")
set_toolchains("msvc")
set_encodings("utf-8")
set_defaultmode("debug")

if is_mode("debug") then 
    binDir = path.join(os.projectdir(), "bin/Debug/")
else 
    binDir = path.join(os.projectdir(), "bin/Release/")
end 

-- 添加系统依赖库
add_syslinks("d3d12", "dxgi", "d3dcompiler", "dxguid", "user32")

-- 添加DXC
add_includedirs("ThridParty/dxc/include")
if is_arch("x64") then
    add_linkdirs("ThridParty/dxc/lib")
elseif is_arch("x86") then
    add_linkdirs("ThridParty/dxc/lib")
end
add_links("dxcompiler")

includes("ThridParty/Imgui")

-- 添加需要的依赖包,同时禁用系统包
add_requires("assimp", {system = false})
add_packages("assimp")

includes("rules.lua")
includes("LearnMiniEngine")

includes("Samples/**")