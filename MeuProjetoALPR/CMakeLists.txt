cmake_minimum_required(VERSION 3.10)
project(MeuProjetoALPR)

# Definimos o padrão C++11 (ou mais novo), necessário para std::chrono
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontra as ferramentas necessárias
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)

# Usa pkg-config para encontrar o OpenALPR
pkg_check_modules(OPENALPR REQUIRED openalpr)

# Adiciona os diretórios de include para OpenALPR e cURL
include_directories(
    ${OPENALPR_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# Cria nosso executável
add_executable(meu_alpr src/main.cpp)

# Linka nosso executável com AMBAS as bibliotecas
target_link_libraries(meu_alpr 
    ${OPENALPR_LIBRARIES}
    ${CURL_LIBRARIES}
)

message(STATUS "OpenALPR Libraries: ${OPENALPR_LIBRARIES}")
message(STATUS "CURL Libraries: ${CURL_LIBRARIES}")