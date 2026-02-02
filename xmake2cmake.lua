task("gen_ide_hintonly_cmake")
set_menu({
    usage = "xmake gencmake [options]",
    description = "生成 CMakeLists.txt 并注入 C++ 模块、标准配置及修复链接错误",
    options = {
        {'s', "std", "kv", "23", "指定 C++ 标准版本 (例如: 17, 20, 23). 默认为 23."}
    }
})

on_run(function ()
    import("core.base.option")

    -- 1. 准备参数
    local std_ver = option.get("std") or "23"
    print("target c++ standard: C++" .. std_ver)

    -- 2. 生成 CMakeLists.txt
    print("Executing: xmake project -k cmakelists ...")
    os.exec("xmake project -k cmakelists")

    local cmake_file = "CMakeLists.txt"
    if not os.isfile(cmake_file) then
        raise("Error: CMakeLists.txt failed to generate.")
    end

    local content = io.readfile(cmake_file)
    -- 创建一个全小写的副本用于搜索位置（忽略大小写）
    local content_lower = content:lower()

    -------------------------------------------------------
    -- 修改 1: 在 cmake_minimum_required 后插入 UUID
    -------------------------------------------------------
    local uuid_code = '\n'--'\nset(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")\n'

    -- 查找 cmake_minimum_required(...) 的结束位置
    local s1, e1 = content_lower:find("cmake_minimum_required%s*%b()")

    if s1 and e1 then
        content = content:sub(1, e1) .. uuid_code .. content:sub(e1 + 1)
        content_lower = content:lower() -- 更新副本
        print("check: injected CMAKE_EXPERIMENTAL_CXX_IMPORT_STD")
    else
        print("warning: 'cmake_minimum_required' not found. Appending UUID to header.")
        content = uuid_code .. content
        content_lower = content:lower()
    end

    -------------------------------------------------------
    -- 修改 2: 在 project(...) 后插入 C++ 模块和标准配置
    -------------------------------------------------------
    local config_block = "\n"--\nset(CMAKE_CXX_MODULE_STD 1)\n"

    if content_lower:find("cmake_cxx_standard") then
        print("check: CMAKE_CXX_STANDARD already exists. Skipping standard injection.")
    else
        print(string.format("check: CMAKE_CXX_STANDARD not found. Injecting C++%s...", std_ver))
        local std_block = string.format([[
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD %s)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()
]], std_ver)
        config_block = config_block .. std_block
    end

    local s2, e2 = content_lower:find("project%s*%b()")

    if s2 and e2 then
        content = content:sub(1, e2) .. config_block .. content:sub(e2 + 1)
        print("check: injected CMAKE_CXX_MODULE_STD and standard settings after project()")
    else
        print("warning: 'project(...)' not found! Appending config to file header.")
        content = config_block .. content
    end

    -------------------------------------------------------
    -- 修改 3: [新增] 自动修复 OBJECT 库链接错误
    -------------------------------------------------------
    -- 问题描述: xmake 生成了错误的 set_property(... IMPORTED_OBJECTS target_name)
    -- 导致 CMake 把 target 名字当成相对路径。
    -- 解决方案: 识别这种模式，替换为直接的 target_link_libraries。

    -- 模式匹配解释:
    -- 1. add_library(temp_name OBJECT IMPORTED GLOBAL)
    -- 2. set_property(TARGET temp_name PROPERTY IMPORTED_OBJECTS real_lib_name)
    -- 3. target_link_libraries(main_target PRIVATE temp_name)
    -- 注意: %s- 匹配零个或多个空白符（含换行），%b() 匹配括号内容不太适用这里因为要提取内容

    local fix_pattern = 'add_library%((target_objectfiles_[%w_]+) OBJECT IMPORTED GLOBAL%)%s-' ..
                        'set_property%(TARGET %1 PROPERTY IMPORTED_OBJECTS%s-([%w_%.]+)%s-%)%s-' ..
                        'target_link_libraries%(([%w_%.]+)%s-PRIVATE %1%)'

    -- 执行全局替换
    local new_content, count = content:gsub(fix_pattern, function(temp_target, real_lib, main_target)
        print(string.format("check: fixing link error for target '%s' -> linking '%s' directly", main_target, real_lib))
        -- 生成正确的链接语句
        return string.format("target_link_libraries(%s PRIVATE %s)", main_target, real_lib)
    end)

    if count > 0 then
        content = new_content
        print(string.format("success: fixed %d instance(s) of incorrect OBJECT library linking.", count))
    else
        print("info: no incorrect OBJECT library linking patterns found (or pattern mismatch).")
    end

    -- 3. 写回文件
    io.writefile(cmake_file, content)
    print("success: CMakeLists.txt updated.")
end)
task_end()