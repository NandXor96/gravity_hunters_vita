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
endif()

if(BUILD_VITA)
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
    ${SDL2_STATIC_LIBRARIES} ${FREETYPE_LIBRARIES}
    SDL2_image freetype bz2 SDL2_ttf freetype webpdemux webp png jpeg z pthread m)
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
    FILE ./assets/images/spaceship.png assets/images/spaceship.png
    FILE ./assets/images/enemies_atlas.png assets/images/enemies_atlas.png
    FILE ./assets/images/splashscreen.png assets/images/splashscreen.png
  )
else()
  find_package(SDL2 REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2IMAGE REQUIRED SDL2_image)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES} SDL2_ttf m)
  target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_PC)
endif()

# Convenience custom targets for make pc / make vita if using Unix Makefiles generator
add_custom_target(pc COMMAND ${CMAKE_COMMAND} -DBUILD_VITA=OFF -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/pc && ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/pc DEPENDS ${PROJECT_NAME})
add_custom_target(vita COMMAND ${CMAKE_COMMAND} -DBUILD_VITA=ON  -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/vita && ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/vita)
