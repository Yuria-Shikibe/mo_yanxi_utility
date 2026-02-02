add_rules("mode.debug", "mode.release")
set_arch("x64")
set_encodings("utf-8")
set_project("mo_yanxi.utility")

-- local available_components = {
--     "algo",
--     "concurrent",
--     "container",
--     "deprecated",
--     "dim2",
--     "general",
--     "generic",
--     "io",
--     "math",
--     "pointer",
--     "string",
-- }
--
-- function mo_yanxi_define_options()
--     for _, name in ipairs(available_components) do
--         option("use_" .. name)
--             set_default(true) -- 默认是否开启，可根据需求改为 false
--             set_showmenu(true)
--             set_description("Enable " .. name .. " component")
--         option_end()
--     end
-- end
--
-- mo_yanxi_define_options()
--
-- function mo_yanxi_get_enabled_components()
--     local enabled = {}
--     for _, name in ipairs(available_components) do
--         if has_config("use_" .. name) then
--             table.insert(enabled, name)
--         end
--     end
--     return enabled;
-- end


option("add_legacy")
    set_default(false) -- 默认是否开启，可根据需求改为 false
    set_showmenu(true)
    set_description("add legacy component")
option_end()

if is_plat("windows") then
    if is_mode("debug") then
        set_runtimes("MDd")
    else
        set_runtimes("MD")
    end
else
    set_runtimes("c++_shared")
end

rule("mo_yanxi.settings")
on_load(function (target)
    -- 设置 C++ 标准
    target:set("languages", "c++latest")
    target:set("policy", "build.c++.modules", true)

    -- 警告与优化
    target:set("warnings", "all", "pedantic")
    target:add("vectorexts", "avx", "avx2")

    -- 自动根据模式处理 Runtime (MD/MDd)
    if target:is_mode("debug") then
        target:set("runtimes", "MDd")
        target:add("defines", "MO_YANXI_UTILITY_ENABLE_CHECK=1")
    else
        target:set("runtimes", "MD")
        target:add("defines", "MO_YANXI_UTILITY_ENABLE_CHECK=0")
    end
end)
rule_end()


function mo_yanxi_utility_use_comp(component_names)
    local include_root = path.join("src", "utility")



    if(component_names == nil or #component_names == 0) then
        add_files(path.join(include_root, "**.ixx"), {public = true})
        return
    end

    add_files(path.join(include_root, "general", "*.ixx"))

    for _, name in ipairs(component_names) do
        local dir = path.join(include_root, name);
        if(os.isdir(dir)) then
            add_files(path.join(dir, "**.ixx"), {public = true})
        end
    end
end



target("mo_yanxi.utility")
    set_kind("object")
    set_languages("c++23")
    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")

    if is_mode("debug") then
        add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=1")
    else
        add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=0")
    end

    add_includedirs("include", {public = true})

    add_files("./src/utility/**.ixx", {public = true})

    if has_config("add_legacy") then
        add_files("./src/legacy/**.ixx", {public = true})
    end
target_end()

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