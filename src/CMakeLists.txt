project(ER_EzMod)

set(CMAKE_CXX_STANDARD 17)
add_library(${PROJECT_NAME} SHARED
    dllmain.cpp
    # inih
    mods/ini.c mods/ini.h
    # Mod base files
    mods/modutils.cpp mods/modutils.h
    mods/moddef.cpp mods/moddef.h
    # Mods
    mods/charflags.cpp
    mods/droprate.cpp
    mods/fasterrespawn.cpp
    mods/nohitbox.cpp
    mods/noruneloss.cpp
    mods/pausegame.cpp
    mods/removechromaticaberration.cpp
    mods/removevignette.cpp
    mods/rideanywhere.cpp
    mods/runerate.cpp
    mods/skipintro.cpp
    mods/summonanywhere.cpp mods/unlockfps.cpp)
target_compile_definitions(${PROJECT_NAME} PUBLIC INI_ALLOW_NO_VALUE=1)
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_NAME "EzMod"
    RUNTIME_OUTPUT_NAME "EzMod")

install(DIRECTORY ../config/
    DESTINATION .
    )
install(FILES ../README.md ../LICENSE
    DESTINATION .
    )
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
    )
string(TIMESTAMP CPACK_PACKAGE_VERSION "%Y%m%d" UTC)
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
