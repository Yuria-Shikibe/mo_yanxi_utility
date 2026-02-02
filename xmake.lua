add_rules("mode.debug", "mode.release")
set_arch("x64")
set_encodings("utf-8")
set_project("mo_yanxi.utility")

local available_components = {
    "algo",
    "concurrent",
    "container",
    "deprecated",
    "dim2",
    "general",
    "generic",
    "io",
    "math",
    "pointer",
    "string",
}

function mo_yanxi_define_options()
    for _, name in ipairs(available_components) do
        option("with_" .. name)
            set_default(true) -- 默认是否开启，可根据需求改为 false
            set_showmenu(true)
            set_description("Enable " .. name .. " component")
        option_end()
    end
end

-- 【新增】获取当前配置下启用了哪些组件
function mo_yanxi_get_enabled_components()
    local enabled = {}
    for _, name in ipairs(available_components) do
        if has_config("with_" .. name) then
            table.insert(enabled, name)
        end
    end
ret


if is_plat("windows") then
    if is_mode("debug") then
        set_runtimes("MDd")
    else
        set_runtimes("MD")
    end
else
    set_runtimes("c++_shared")
end

includes("config.lua");

target("mo_yanxi.utility")
    set_kind("object")
    set_languages("c++23")
    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")
target_end()

mo_yanxi_utility_add_comp_to("mo_yanxi.utility")

target("mo_yanxi.utility.test")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++23")
    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")

    add_files("test/**.cpp")
    add_deps("mo_yanxi.utility")
target_end()

includes("xmake2cmake.lua")