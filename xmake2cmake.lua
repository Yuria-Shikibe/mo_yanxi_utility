task("gen_ide_hintonly_cmake")
set_menu({
    usage = "xmake gencmake [options]",
    description = "生成 CMakeLists.txt 并注入 C++ 模块与标准配置",
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
        -- 在匹配到的末尾插入
        content = content:sub(1, e1) .. uuid_code .. content:sub(e1 + 1)
        -- 更新 lower 副本以便后续搜索索引准确（虽然 project 一般在后面，但为了严谨）
        content_lower = content:lower()
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

    -- 检查是否需要插入 C++ 标准配置
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

    -- 查找 project(...) 的结束位置
    local s2, e2 = content_lower:find("project%s*%b()")

    if s2 and e2 then
        -- 插入到 project(...) 闭括号后面
        content = content:sub(1, e2) .. config_block .. content:sub(e2 + 1)
        print("check: injected CMAKE_CXX_MODULE_STD and standard settings after project()")
    else
        -- 如果还是找不到 project，就尝试插入到 uuid 代码块后面，或者文件头部
        print("warning: 'project(...)' not found! Appending config to file header.")
        content = config_block .. content
    end

    -- 3. 写回文件
    io.writefile(cmake_file, content)
    print("success: CMakeLists.txt updated.")
end)
task_end()