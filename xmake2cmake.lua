task("gen_ide_hintonly_cmake")
set_menu({
    usage = "xmake gencmake [options]",
    description = "生成 CMakeLists.txt 并通过管道处理注入配置（C++ 模块、标准库、路径修复等）",
    options = {
        { 's', "std", "kv", "23", "指定 C++ 标准版本 (例如: 17, 20, 23). 默认为 23." },
        { 'm', "mode", "kv", "debug", "指定最终替换的 output directory 模式名称. 默认为 debug." },
        { 'p', "pass", "kv", "", "透传给 xmake f 的额外参数" }
    }
})

on_run(function()
    import("core.base.option")
    import("core.project.config")

    -----------------------------------------------------------------------
    -- 1. 上下文构建 (Context Builder)
    -- 集中处理参数解析和环境配置
    -----------------------------------------------------------------------
    local function build_context()
        config.load()     -- 加载项目配置

        local ctx = {
            std_ver     = option.get("std") or "23",
            mode_name   = option.get("mode") or "debug",
            pass_args   = option.get("pass") or "",
            project_dir = os.projectdir(),
            cmake_file  = "CMakeLists.txt",
            base_dirs   = { os.projectdir(), }     -- 默认包含根目录
        }

        -- 获取自定义工具路径
        local util_path = config.get("spec_mo_yanxi_utility_path")
        if util_path and #util_path > 0 then
            table.insert(ctx.base_dirs, path.directory(util_path))
            print("info: context loaded utility path: " .. path.directory(util_path))
        end

        return ctx
    end

    -----------------------------------------------------------------------
    -- 2. 核心逻辑：文件内容处理器 (Content Processors)
    -- 每个函数只做一件具体的文本修改任务
    -----------------------------------------------------------------------
    local processors = {}

    -- 处理器：注入 UUID (针对 C++ Modules 的实验性支持)
    table.insert(processors, function(content, ctx)
        -- 这里的 uuid_code 根据你原始内容保留，如果是空的换行符可以自行调整
        local uuid_code = '\n'     -- '\nset(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")\n'
        local s, e = content:lower():find("cmake_minimum_required%s*%b()")

        if s and e then
            print("patch: injected UUID/Header after cmake_minimum_required")
            return content:sub(1, e) .. uuid_code .. content:sub(e + 1)
        else
            print("patch: 'cmake_minimum_required' not found, prepending header")
            return uuid_code .. content
        end
    end)

    -- 处理器：注入 C++ 标准配置
    table.insert(processors, function(content, ctx)
        if content:lower():find("cmake_cxx_standard") then
            print("patch: CMAKE_CXX_STANDARD exists, skipping injection")
            return content
        end

        local config_block = string.format([[
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD %s)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()
]], ctx.std_ver)

        local s, e = content:lower():find("project%s*%b()")
        if s and e then
            print("patch: injected C++" .. ctx.std_ver .. " settings after project()")
            return content:sub(1, e) .. "\n" .. config_block .. content:sub(e + 1)
        else
            return config_block .. content
        end
    end)

    -- 处理器：清理 set_property
    table.insert(processors, function(content, ctx)
        local new_content, count = content:gsub("set_property%s*%b()%s*", "")
        if count > 0 then
            print(string.format("patch: removed %d set_property statements", count))
        end
        return new_content
    end)

    -- 处理器：修复输出目录 (cmake_gen -> debug/release)
    table.insert(processors, function(content, ctx)
        if ctx.mode_name == "cmake_gen" then return content end

        local new_content, count = content:gsub("/cmake_gen\"", "/" .. ctx.mode_name .. "\"")
        if count > 0 then
            print(string.format("patch: fix output dir 'cmake_gen' -> '%s' (%d times)", ctx.mode_name, count))
        end
        return new_content
    end)

    -- 处理器：注入 BASE_DIRS 到 C++ Modules 定义中
    table.insert(processors, function(content, ctx)
        if #ctx.base_dirs == 0 then return content end

        -- 构建路径字符串
        local paths = {}
        for _, dir in ipairs(ctx.base_dirs) do
            -- 统一转为 Unix 风格路径以避免 CMake 转义问题
            table.insert(paths, '"' .. dir:gsub("\\", "/") .. '"')
            print("add base dir: " .. dir)
        end
        local base_dirs_str = "BASE_DIRS " .. table.concat(paths, " ") .. " "

        -- 正则替换：插入到 FILE_SET CXX_MODULES 和 FILES 之间
        local pattern = "(FILE_SET%s+CXX_MODULES%s+)(FILES)"
        local new_content, count = content:gsub(pattern, "%1" .. base_dirs_str .. "%2")

        if count > 0 then
            print(string.format("patch: injected BASE_DIRS into %d target sources", count))
        else
            print("warning: failed to inject BASE_DIRS (pattern not found)")
        end
        return new_content
    end)

    -----------------------------------------------------------------------
    -- 3. 主执行流程
    -----------------------------------------------------------------------
    local ctx = build_context()

    print("========================================")
    print("Target Standard: C++" .. ctx.std_ver)
    print("Output Mode:     " .. ctx.mode_name)
    print("========================================")

    -- 步骤 A: 执行 xmake 命令生成原始文件
    -- 使用 os.execv 可以更安全地处理参数中的空格，但 xmake f通常用 string
    os.exec("xmake f -y --mode=cmake_gen " .. ctx.pass_args)
    os.exec("xmake project -k cmakelists")
    -- 恢复原来的模式配置，以免影响后续编译
    os.exec("xmake f -y --mode=" .. ctx.mode_name .. " " .. ctx.pass_args)

    if not os.isfile(ctx.cmake_file) then
        raise("Error: CMakeLists.txt failed to generate.")
    end

    -- 步骤 B: 读取文件
    local content = io.readfile(ctx.cmake_file)

    -- 步骤 C: 管道处理 (依次应用所有处理器)
    for _, processor in ipairs(processors) do
        content = processor(content, ctx)
    end

    -- 步骤 D: 写回文件
    io.writefile(ctx.cmake_file, content)
    print("success: CMakeLists.txt updated successfully.")
end)
