
function mo_yanxi_utility_add_comp_to(target_name, component_names)
    if target_name == nil then
        print("MoYanxi: Invalid Target: ", target_name)
        os.exit(-1, true)
    end

    target(target_name)
        local root = os.scriptdir()
        local include_root = path.join(root, "src", "utility")

        set_policy("build.c++.modules", true)

        if is_mode("debug") then
            add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=1")
        else
            add_defines("MO_YANXI_UTILITY_ENABLE_CHECK=0")
        end

        add_includedirs(path.join(root, "include"), {public = true})

        if(component_names == nil or #component_names == 0) then
            add_files(path.join(include_root, "**.ixx"), {public = true})
            on_load(function(t)
                print("MoYanxi: All Utility Component Added")
            end)

            return
        end

        add_files(path.join(include_root, "general", "*.ixx"))

        for _, name in ipairs(component_names) do
            local dir = path.join(include_root, name);
            if(os.isdir(dir)) then
                add_files(path.join(dir, "**.ixx"), {public = true})
                on_load(function(t)
                    print("MoYanxi: Utility Component Added: ", name)
                end)
            else
                on_load(function(t)
                    print("MoYanxi: Invalid Component: ", name)
                end)

            end
        end
    target_end()
end

function mo_yanxi_utility_import_default(target_name)
    target(target_name)
        set_kind("static")
        set_languages("c++latest")

        set_warnings("all")
        set_warnings("pedantic")
        add_vectorexts("avx", "avx2")
    target_end()

    mo_yanxi_utility_add_comp_to(target_name)
end