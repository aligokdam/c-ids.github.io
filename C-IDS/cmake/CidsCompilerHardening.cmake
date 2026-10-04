# CidsCompilerHardening.cmake
#
# Central place for the "Zero-Trust coding" compiler policy described in
# docs/SECURITY_POLICY.md. Any target can opt in with:
#
#   cids_apply_hardening(<target>)
#
function(cids_apply_hardening TARGET_NAME)

    set(_warn_flags "")
    set(_hardening_flags "")
    set(_sanitizer_flags "")

    if(MSVC)
        list(APPEND _warn_flags /W4 /permissive- /sdl)
        if(CIDS_TREAT_WARNINGS_AS_ERRORS)
            list(APPEND _warn_flags /WX)
        endif()
        if(CIDS_ENABLE_HARDENING)
            list(APPEND _hardening_flags /GS /guard:cf /Qspectre)
            target_link_options(${TARGET_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT /CETCOMPAT)
        endif()
    else() # Clang / GCC (Linux, macOS, iOS toolchains)
        list(APPEND _warn_flags
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused
            -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion
        )
        if(CIDS_TREAT_WARNINGS_AS_ERRORS)
            list(APPEND _warn_flags -Werror)
        endif()
        if(CIDS_ENABLE_HARDENING)
            list(APPEND _hardening_flags
                -fstack-protector-strong
                -D_FORTIFY_SOURCE=2
                -fPIE
            )
            target_link_options(${TARGET_NAME} PRIVATE -pie)
        endif()
        if(CIDS_ENABLE_SANITIZERS)
            list(APPEND _sanitizer_flags
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
                $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
            )
            target_link_options(${TARGET_NAME} PRIVATE
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            )
        endif()
    endif()

    target_compile_options(${TARGET_NAME} PRIVATE
        ${_warn_flags} ${_hardening_flags} ${_sanitizer_flags}
    )

    # clang-tidy hook: static analysis enforces the "no raw owning pointers"
    # rule from docs/SECURITY_POLICY.md at every build, not just in CI.
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set_target_properties(${TARGET_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
        )
    endif()

endfunction()
