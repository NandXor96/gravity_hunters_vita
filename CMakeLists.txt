cmake_minimum_required(VERSION 3.16)

option(BUILD_VITA "Build for PlayStation Vita" OFF)

if(BUILD_VITA)
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
      set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
      message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
  endif()
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

project(gravity_hunters)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.c)
add_executable(${PROJECT_NAME} ${SRC_FILES})

if(BUILD_VITA)
  target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_VITA)
  find_package(SDL2 REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME}
  SDL2_image SDL2_ttf SDL2_mixer freetype
    modplug mpg123 vorbisfile vorbis ogg opusfile opus xmp
    bz2 webpdemux webp png jpeg
    ${SDL2_STATIC_LIBRARIES} ${FREETYPE_LIBRARIES}
    stdc++
    z pthread m
    SceAppUtil_stub SceSysmodule_stub)
  set(VITA_APP_NAME "Gravity Hunters")
  set(VITA_TITLEID  "GRAVHUNT1")
  set(VITA_VERSION  "01.00")
  vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
  vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE ./sce_sys/icon0.png sce_sys/icon0.png
    FILE ./sce_sys/pic0.png sce_sys/pic0.png
    FILE ./sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE ./sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE ./sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml

    FILE ./assets/fonts/comic_shanns2.ttf assets/fonts/comic_shanns2.ttf

    FILE ./assets/images/hud_atlas.png assets/images/hud_atlas.png
    FILE ./assets/images/planets_atlas.png assets/images/planets_atlas.png
    FILE ./assets/images/player.png assets/images/player.png
    FILE ./assets/images/enemies_atlas.png assets/images/enemies_atlas.png
    FILE ./assets/images/splashscreen.png assets/images/splashscreen.png
    FILE ./assets/images/logo.png assets/images/logo.png

    FILE ./assets/audio/bg_loop.ogg assets/audio/bg_loop.ogg
    FILE ./assets/audio/shot.ogg assets/audio/shot.ogg
    FILE ./assets/audio/shot2.ogg assets/audio/shot2.ogg

    FILE ./assets/buttons/Vita_Circle.png assets/buttons/Vita_Circle.png
    FILE ./assets/buttons/Vita_Cross.png assets/buttons/Vita_Cross.png

    FILE ./assets/levels/01_-_Intro.lvl assets/levels/01_-_Intro.lvl
    FILE ./assets/levels/02_-_First_mission.lvl assets/levels/02_-_First_mission.lvl
    FILE ./assets/levels/03_-_Pest_control.lvl assets/levels/03_-_Pest_control.lvl
    FILE ./assets/levels/04_-_Ambush.lvl assets/levels/04_-_Ambush.lvl
    FILE ./assets/levels/05_-_Magellanic.lvl assets/levels/05_-_Magellanic.lvl
    FILE ./assets/levels/06_-_Oat_Milk_Way.lvl assets/levels/06_-_Oat_Milk_Way.lvl
    FILE ./assets/levels/07_-_Birds_in_Space.lvl assets/levels/07_-_Birds_in_Space.lvl
    FILE ./assets/levels/08_-_Trickshots.lvl assets/levels/08_-_Trickshots.lvl
    FILE ./assets/levels/09_-_Space_Bees!!!.lvl assets/levels/09_-_Space_Bees!!!.lvl
    FILE ./assets/levels/10_-_Itchy.lvl assets/levels/10_-_Itchy.lvl
    FILE ./assets/levels/11_-_Nova_Nomads.lvl assets/levels/11_-_Nova_Nomads.lvl
    FILE ./assets/levels/12_-_Pirate_Drones.lvl assets/levels/12_-_Pirate_Drones.lvl

    FILE ./assets/credits.txt assets/credits.txt
    FILE ./assets/help.txt assets/help.txt

    FILE ./licenses/game_license.txt licenses/assets_license.txt
    FILE ./licenses/bzip2_license.txt licenses/bzip2_license.txt
    FILE ./licenses/font_license.txt licenses/font_license.txt
    FILE ./licenses/freetype_license.txt licenses/freetype_license.txt
    FILE ./licenses/libjpeg_license.txt licenses/libjpeg_license.txt
    FILE ./licenses/libpng_license.txt licenses/libpng_license.txt
    FILE ./licenses/libwebp_license.txt licenses/libwebp_license.txt
    FILE ./licenses/sdl2_image_license.txt licenses/sdl2_image_license.txt
    FILE ./licenses/sdl2_license.txt licenses/sdl2_license.txt
    FILE ./licenses/sdl2_ttf_license.txt licenses/sdl2_ttf_license.txt
    FILE ./licenses/zlib_license.txt licenses/zlib_license.txt
  )
else()
  # PC Build - nur ausführen wenn BUILD_VITA=OFF
  find_package(SDL2 REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2IMAGE REQUIRED SDL2_image)
  pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)
  pkg_check_modules(SDL2MIXER REQUIRED SDL2_mixer)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS} ${SDL2MIXER_INCLUDE_DIRS})
  if(SDL2IMAGE_LIBRARY_DIRS)
    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2IMAGE_LIBRARY_DIRS})
  endif()
  if(SDL2TTF_LIBRARY_DIRS)
    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2TTF_LIBRARY_DIRS})
  endif()
  if(SDL2MIXER_LIBRARY_DIRS)
    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2MIXER_LIBRARY_DIRS})
  endif()
  set(PC_LINK_LIBS ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES} ${SDL2TTF_LIBRARIES} ${SDL2MIXER_LIBRARIES})
  list(REMOVE_DUPLICATES PC_LINK_LIBS)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PC_LINK_LIBS})
  target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_PC)
endif()

# Custom targets - diese werden beim ersten cmake-Lauf definiert, unabhängig von BUILD_VITA
add_custom_target(pc 
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/pc
  COMMAND ${CMAKE_COMMAND} -DBUILD_VITA=OFF -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/pc 
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/pc
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(vita 
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/vita
  COMMAND ${CMAKE_COMMAND} -DBUILD_VITA=ON -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/vita 
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/vita
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)