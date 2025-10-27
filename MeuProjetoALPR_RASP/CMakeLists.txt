cmake_minimum_required(VERSION 3.10)
project(MeuProjetoALPR_RASP)

# Definimos o padrão C++11 (ou mais novo)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontra as bibliotecas necessárias: OpenCV, cURL e OpenALPR (via pkg-config)
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(OPENALPR REQUIRED openalpr)

# Adiciona todos os diretórios de "include"
include_directories(
    ${OPENALPR_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src # Para encontrar json.hpp
)

# Cria nosso executável para tempo real
add_executable(alpr_realtime src/main.cpp)

# Linka o executável com TODAS as bibliotecas necessárias
target_link_libraries(alpr_realtime
    ${OPENALPR_LIBRARIES}
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
)

message(STATUS "OpenALPR Encontrado: ${OPENALPR_LIBRARIES}")
message(STATUS "OpenCV Encontrado: ${OpenCV_LIBS}")
message(STATUS "CURL Encontrado: ${CURL_LIBRARIES}")