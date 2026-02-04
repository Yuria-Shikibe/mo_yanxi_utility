task("gen_ide_hintonly_cmake")
set_menu({
    usage = "xmake gencmake [options]",
    description = "生成 CMakeLists.txt 并注入 C++ 模块、标准配置及修复链接错误",
    options = {
        {'s', "std", "kv", "23", "指定 C++ 标准版本 (例如: 17, 20, 23). 默认为 23."},
        {'m', "mode", "kv", "debug", "指定替换 target output directory 的模式名称 (例如: debug, release). 默认为 debug."}
    }
})

on_run(function ()
    import("core.base.option")

    -- 1. 准备参数
    local std_ver = option.get("std") or "23"
    local mode_name = option.get("mode") or "debug" -- 获取 mode 参数

    print("target c++ standard: C++" .. std_ver)
    print("target output mode name: " .. mode_name)

    -- 2. 生成 CMakeLists.txt
    -- 注意：这里保持使用 cmake_gen 进行生成，以确保生成的文本中包含确定的关键字供后续替换
    print("Executing: xmake project -k cmakelists ...")
    os.exec("xmake f -y -m cmake_gen")
    os.exec("xmake project -k cmakelists")
    os.exec("xmake f -y -m debug")

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
    -- 修改 3: [修改] 直接删除 set_property
    -------------------------------------------------------
    -- 之前的逻辑是尝试修复链接，现在根据需求直接移除 set_property 语句。
    -- %s* 匹配空白，%b() 匹配括号及其内容

    local count
    content, count = content:gsub("set_property%s*%b()%s*", "")

    if count > 0 then
        print(string.format("success: removed %d set_property statement(s).", count))
    else
        print("info: no set_property statements found.")
    end

    -------------------------------------------------------
    -- 修改 4: [新增] 替换 set_target_properties 中的输出目录
    -------------------------------------------------------
    -- 将 ".../cmake_gen" 替换为 ".../<mode>"
    -- 这样可以修改 RUNTIME_OUTPUT_DIRECTORY 等属性
    if mode_name ~= "cmake_gen" then
        local replace_count
        -- 匹配规则：以 /cmake_gen" 结尾的字符串
        -- 这里的双引号是 CMake 语法的一部分，确保我们修改的是路径末尾
        content, replace_count = content:gsub("/cmake_gen\"", "/" .. mode_name .. "\"")

        if replace_count > 0 then
            print(string.format("success: replaced output directory 'cmake_gen' with '%s' (%d occurrences).", mode_name, replace_count))
        else
            print("warning: 'cmake_gen' path not found in properties. Check xmake configuration.")
        end
    end

    -- 3. 写回文件
    io.writefile(cmake_file, content)
    print("success: CMakeLists.txt updated.")
end)
task_end()