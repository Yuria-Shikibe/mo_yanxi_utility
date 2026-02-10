add_rules("mode.debug", "mode.release")
set_arch("x64")
set_encodings("utf-8")
set_project("mo_yanxi.utility")

option("add_legacy") --Used for libc++/clang, missing some of c++23 facilities
    set_default(false)
    set_showmenu(true)
    set_description("add legacy component")
option_end()

option("add_latest") --Used for libc++/clang, missing some of c++23 facilities
    set_default(true)
option_end()


option("add_test")
    set_default(false)
    set_description("Add google test target")
option_end()

option("add_benchmark")
    set_default(false)
    set_description("Add add_benchmark target")
option_end()


option("use_libcxx")
    add_deps("toolchain")
    on_check(function (option)
        option:enable(get_config("toolchain") == "clang" and not is_plat("windows"))
    end)
    set_description("Use libc++")
option_end()

if not get_config("runtimes") then
    if is_plat("windows") then
        set_runtimes(is_mode("debug") and "MDd" or "MD")
    else
        set_runtimes("c++_shared")
    end
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

if has_config("add_test") then add_requires("gtest") end
if has_config("add_benchmark") then add_requires("benchmark") end

if (get_config("toolchain") ~= "msvc") and has_config("use_libcxx") then
    add_requireconfs("*", {configs = {cxflags = "-stdlib=libc++", ldflags = {"-stdlib=libc++", "-lc++abi", "-lunwind"}}})
    add_cxflags("-stdlib=libc++")
    add_ldflags("-stdlib=libc++", "-lc++abi", "-lunwind")
end

target("mo_yanxi.utility")
    set_kind("object")
    set_languages("c++23")
    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")

    if is_mode("debug") then
        add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=1", {public = true})
    else
        add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=0", {public = true})
    end

    add_includedirs("include", {public = true})
    add_installfiles("include/(**.hpp)", {prefixdir = "include"})

    add_files("./src/utility/**.ixx", {public = true})

    if has_config("add_legacy") then
        add_files("./src/legacy/**.ixx", {public = true})
    end

    if has_config("add_latest") then
        add_files("./src/latest/**.ixx", {public = true})
    end
target_end()


target("mo_yanxi.utility.test")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++23")
    set_optimize("fastest")

    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")
    add_deps("mo_yanxi.utility")
    add_packages("gtest")

    add_files("test/**.cpp")

    set_enabled(has_config("add_test"))
target_end()


target("mo_yanxi.utility.benchmark")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++23")
    set_optimize("fastest")

    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")
    add_deps("mo_yanxi.utility")
    add_packages("benchmark")

    add_files("benchmark/**.cpp")
    set_enabled(has_config("add_benchmark"))
target_end()

includes("xmake2cmake.lua")