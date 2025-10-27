cmake_minimum_required(VERSION 3.10)
project(MeuProjetoALPR_RASP_Foto) # Nome do projeto alterado

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(OPENALPR REQUIRED openalpr)

include_directories(
    ${OPENALPR_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Nome do executável alterado
add_executable(alpr_captura_foto src/main.cpp)

target_link_libraries(alpr_captura_foto
    ${OPENALPR_LIBRARIES}
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
)

message(STATUS "OpenALPR Encontrado: ${OPENALPR_LIBRARIES}")
message(STATUS "OpenCV Encontrado: ${OpenCV_LIBS}")
message(STATUS "CURL Encontrado: ${CURL_LIBRARIES}")
