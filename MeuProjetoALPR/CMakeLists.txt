cmake_minimum_required(VERSION 3.10)
project(MeuProjetoALPR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontra a ferramenta pkg-config
find_package(PkgConfig REQUIRED)

# Usa pkg-config para encontrar o OpenALPR que instalamos
pkg_check_modules(OPENALPR REQUIRED openalpr)

# Inclui os diretórios de cabeçalho do OpenALPR
include_directories(${OPENALPR_INCLUDE_DIRS})

# Cria nosso executável
add_executable(meu_alpr src/main.cpp)

# Linka nosso executável com a biblioteca OpenALPR
target_link_libraries(meu_alpr ${OPENALPR_LIBRARIES})

message(STATUS "OpenALPR Encontrado:")
message(STATUS "  Includes: ${OPENALPR_INCLUDE_DIRS}")
message(STATUS "  Libraries: ${OPENALPR_LIBRARIES}")
