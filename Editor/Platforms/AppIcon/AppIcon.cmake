# The icon of the o2 editor's executable: the icon resource on Windows, the Finder icon on Mac. The Dock and the window
# icons are set at the start by EditorApplication (Application::SetApplicationIcon, o2Editor_256.png)
set(O2_EDITOR_APP_ICON_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(o2_editor_app_icon target)
    if (EMSCRIPTEN)
        return()
    endif()

    if (WIN32)
        target_sources(${target} PRIVATE "${O2_EDITOR_APP_ICON_DIR}/o2Editor.rc")
    elseif (APPLE)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND osascript -l JavaScript "${O2_EDITOR_APP_ICON_DIR}/SetFileIcon.js"
                    "${O2_EDITOR_APP_ICON_DIR}/o2Editor.icns" "$<TARGET_FILE:${target}>" > /dev/null || true
            COMMENT "Setting the icon of the editor executable")
    endif()
endfunction()
