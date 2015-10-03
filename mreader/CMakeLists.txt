#=============================================================================
#  MuseScore
#  Music Composition & Notation
#  $Id:$
#
#  Copyright (C) 2011-2013 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

include_directories(
      ${PROJECT_BINARY_DIR}
      ${PROJECT_SOURCE_DIR}
      )

if (APPLE)
      QT5_ADD_RESOURCES (qrc_files
            ${PROJECT_SOURCE_DIR}/mscore/musescore.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Mac.qrc)
else (APPLE)
      QT5_ADD_RESOURCES (qrc_files
            ${PROJECT_SOURCE_DIR}/mscore/musescore.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-MScore.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Gootville.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Bravura.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-MuseJazz.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-FreeSerif.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Free.qrc)
endif (APPLE)

add_executable(
      mreader
      mreader.cpp
      ${qrc_files}
      )

target_link_libraries(
      mreader
      libmscore
      synthesizer
      xmlstream
      qzip
      z
      freetype
      ${QT_LIBRARIES}
      )

set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

if (APPLE)
   set(CMAKE_CXX_FLAGS_DEBUG   "-std=c++0x -fPIC -stdlib=libc++ -g")
   set(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -fPIC -stdlib=libc++ -O2 -DNDEBUG -DQT_NO_DEBUG")
else (APPLE)
   if (MINGW)
      # -mno-ms-bitfields see #22048
      set(CMAKE_CXX_FLAGS_DEBUG   "-std=gnu++0x -mno-ms-bitfields -g")
      set(CMAKE_CXX_FLAGS_RELEASE "-std=gnu++0x -mno-ms-bitfields -O2 -DNDEBUG -DQT_NO_DEBUG")
   else (MINGW)
      set(CMAKE_CXX_FLAGS_DEBUG   "-std=gnu++0x -fPIC -fPIE -g")
      set(CMAKE_CXX_FLAGS_RELEASE "-std=gnu++0x -fPIC -O2 -DNDEBUG -DQT_NO_DEBUG")
   endif (MINGW)
endif(APPLE)


string(REPLACE ";" ";-I" INC "${QT_INCLUDES}")
