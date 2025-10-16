#include <iostream>
#include <alpr.h>
#include <vector>  // Para criar nossa "tabela" de placas
#include <string>  // Para usar std::string

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Uso: ./meu_alpr <caminho_para_imagem>" << std::endl;
        return 1;
    }

    // --- TABELA DE PLACAS ROUBADAS (NOME MAIS CLARO) ---
    std::vector<std::string> placas_roubadas = {
        "GZN1B34",
        "BRA2E19",
        "PBT5340"
    };

    std::string image_path = argv[1];

    alpr::Alpr openalpr("eu", "/etc/openalpr/openalpr.conf");
    openalpr.setTopN(3); // Pedimos os 3 melhores resultados para aumentar a chance de acerto

    if (!openalpr.isLoaded()) {
        std::cerr << "Erro ao carregar a biblioteca OpenALPR" << std::endl;
        return 1;
    }

    alpr::AlprResults results = openalpr.recognize(image_path);

    bool roubada = false;

    // Itera sobre as placas físicas encontradas na imagem
    for (int i = 0; i < results.plates.size(); i++) {
        alpr::AlprPlateResult plate = results.plates[i];
        
        std::cout << "\nAnalisando Placa " << i+1 << "..." << std::endl;

        // *** CORREÇÃO PRINCIPAL: Itera sobre TODOS os N melhores resultados ***
        for (int k = 0; k < plate.topNPlates.size(); k++) {
            alpr::AlprPlate candidate = plate.topNPlates[k];
            std::string placa_lida = candidate.characters;
            float confianca = candidate.overall_confidence;

            std::cout << "  - Tentativa " << k+1 << ": " << placa_lida 
                      << " (Confianca: " << confianca << "%)" << std::endl;

            // --- SUGESTÃO: Adiciona um filtro de confiança ---
            if (confianca < 80.0) {
                std::cout << "      -> Confianca muito baixa, ignorando." << std::endl;
                continue; // Pula para a próxima tentativa
            }

            // Itera sobre nossa tabela de placas roubadas
            for (const std::string& placa_roubada : placas_roubadas) {
                if (placa_lida == placa_roubada) {
                    std::cout << ">>> ALERTA: Placa '" << placa_roubada << "' encontrada na lista de roubados!" << std::endl;
                    roubada = true;
                    break; // Sai do loop de verificação (placas_roubadas)
                }
            }
            if (roubada) {
                break; // Sai do loop de tentativas (topNPlates)
            }
        }
        
        if (roubada) {
            break; // Sai do loop de placas físicas
        }
    }

    // --- TOMADA DE DECISÃO FINAL ---
    if (roubada) {
        std::cout << "\nACAO: Enviando notificacao as autoridades e ao proprietario!" << std::endl;
    } else {
        std::cout << "\nSTATUS: Tudo OK. Nenhuma placa roubada detectada." << std::endl;
    }

    return 0;
}