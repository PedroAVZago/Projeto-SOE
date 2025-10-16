#include <iostream>
#include <alpr.h>
#include <vector>  // Para criar nossa "tabela" de placas
#include <string>  // Para usar std::string
#include <cctype>  // Necessário para as funções isalpha e isdigit
#include <fstream> // Necessário para ler arquivos (file stream)

/**
 * @brief Valida se uma string de placa corresponde aos formatos brasileiros.
 * (Esta função permanece a mesma)
 */
bool validaFormatoPlaca(const std::string& placa) {
    if (placa.length() != 7) {
        return false;
    }
    bool formatoAntigoValido = 
        isalpha(placa[0]) && isalpha(placa[1]) && isalpha(placa[2]) &&
        isdigit(placa[3]) && isdigit(placa[4]) && isdigit(placa[5]) && isdigit(placa[6]);
    if (formatoAntigoValido) return true;

    bool formatoMercosulValido = 
        isalpha(placa[0]) && isalpha(placa[1]) && isalpha(placa[2]) &&
        isdigit(placa[3]) && isalpha(placa[4]) && isdigit(placa[5]) && isdigit(placa[6]);
    if (formatoMercosulValido) return true;

    return false;
}

/**
 * @brief Carrega uma lista de placas a partir de um arquivo de texto.
 * @param nomeArquivo O caminho para o arquivo de texto.
 * @return Um vetor de strings, onde cada string é uma placa.
 */
std::vector<std::string> carregarPlacasDoArquivo(const std::string& nomeArquivo) {
    std::vector<std::string> placas;
    std::ifstream arquivo(nomeArquivo); // Abre o arquivo para leitura

    if (!arquivo.is_open()) {
        std::cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de placas: " << nomeArquivo << std::endl;
        return placas; // Retorna o vetor vazio
    }

    std::string linha;
    while (std::getline(arquivo, linha)) {
        if (!linha.empty()) { // Garante que não adicionamos linhas em branco
            placas.push_back(linha);
        }
    }

    arquivo.close();
    std::cout << placas.size() << " placas carregadas do arquivo." << std::endl;
    return placas;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Uso: ./meu_alpr <caminho_para_imagem>" << std::endl;
        return 1;
    }

    // --- CARREGAMENTO DAS PLACAS A PARTIR DO ARQUIVO ---
    // O executável está em build/, então precisamos voltar dois diretórios (../../)
    // para chegar à raiz do projeto (PROJETO_SOE)
    std::string arquivoDePlacas = "../../PLACAS_ROUBADAS.txt";
    std::vector<std::string> placas_roubadas = carregarPlacasDoArquivo(arquivoDePlacas);

    // Se o arquivo não pôde ser aberto ou estava vazio, o programa termina.
    if (placas_roubadas.empty()) {
        std::cerr << "A lista de placas esta vazia ou o arquivo nao foi encontrado. Encerrando." << std::endl;
        return 1;
    }

    std::string image_path = argv[1];

    alpr::Alpr openalpr("eu", "/etc/openalpr/openalpr.conf");
    openalpr.setTopN(5);

    if (!openalpr.isLoaded()) {
        std::cerr << "Erro ao carregar a biblioteca OpenALPR" << std::endl;
        return 1;
    }

    alpr::AlprResults results = openalpr.recognize(image_path);

    bool roubada = false;

    // O restante da lógica permanece exatamente o mesmo
    for (int i = 0; i < results.plates.size(); i++) {
        alpr::AlprPlateResult plate = results.plates[i];
        
        std::cout << "\nAnalisando Placa " << i+1 << "..." << std::endl;

        for (int k = 0; k < plate.topNPlates.size(); k++) {
            alpr::AlprPlate candidate = plate.topNPlates[k];
            std::string placa_lida = candidate.characters;
            float confianca = candidate.overall_confidence;

            std::cout << "  - Tentativa " << k+1 << ": " << placa_lida 
                      << " (Confianca: " << confianca << "%)" << std::endl;

            if (!validaFormatoPlaca(placa_lida)) {
                std::cout << "      -> Formato invalido, ignorando." << std::endl;
                continue;
            }

            if (confianca < 60.0) {
                std::cout << "      -> Confianca muito baixa, ignorando." << std::endl;
                continue;
            }

            for (const std::string& placa_roubada : placas_roubadas) {
                if (placa_lida == placa_roubada) {
                    std::cout << ">>> ALERTA: Placa '" << placa_roubada << "' encontrada na lista de roubados!" << std::endl;
                    roubada = true;
                    break;
                }
            }
            if (roubada) break;
        }
        
        if (roubada) break;
    }

    if (roubada) {
        std::cout << "\nACAO: Enviando notificacao as autoridades e ao proprietario!" << std::endl;
    } else {
        std::cout << "\nSTATUS: Tudo OK. Nenhuma placa roubada detectada." << std::endl;
    }

	return 0;
}