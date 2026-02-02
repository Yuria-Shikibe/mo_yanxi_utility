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

    set_policy("build.c++.modules", true)

    if is_mode("debug") then
        add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=1")
    else
        add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=0")
    end

    add_includedirs("include", {public = true})

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


function mo_yanxi_utility_add_comp_to(target_name, component_names)
    if target_name == nil then
        print("MoYanxi: Invalid Target: ", target_name)
        os.exit(-1, true)
    end

    target(target_name)
        mo_yanxi_utility_use_comp(component_names)
    target_end()
end

function mo_yanxi_utility_import_default(target_name)
    target(target_name)
        set_kind("static")
        set_languages("c++23")
        set_policy("build.c++.modules", true)

        set_warnings("all")
        set_warnings("pedantic")
    target_end()

    mo_yanxi_utility_add_comp_to(target_name)
end