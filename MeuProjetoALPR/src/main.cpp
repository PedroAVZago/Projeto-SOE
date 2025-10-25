#include <iostream>
#include <alpr.h>
#include <vector>
#include <string>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <algorithm>

// --- NOVA ESTRUTURA PARA ARMAZENAR DADOS DE LOCALIZAÇÃO ---
struct LocalInfo {
    std::string latitude = "NAO_DEFINIDO";
    std::string longitude = "NAO_DEFINIDO";
    std::string local_descricao = "Local Desconhecido";
};

/**
 * @brief Função de callback para "engolir" a resposta do curl.
 * Isso impede que o JSON de resposta do Telegram seja impresso no console.
 */
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    // Retorna o número de bytes "lidos" para que o curl pense que foi sucesso
    return size * nmemb;
}

/**
 * @brief Remove espaços em branco do início de uma string.
 */
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

/**
 * @brief Valida se uma string de placa corresponde aos formatos brasileiros.
 */
bool validaFormatoPlaca(const std::string& placa) {
    if (placa.length() != 7) return false;
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
 */
std::vector<std::string> carregarPlacasDoArquivo(const std::string& nomeArquivo) {
    std::vector<std::string> placas;
    std::ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        std::cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de placas: " << nomeArquivo << std::endl;
        return placas;
    }
    std::string linha;
    while (std::getline(arquivo, linha)) {
        if (!linha.empty()) {
            placas.push_back(linha);
        }
    }
    arquivo.close();
    std::cout << placas.size() << " placas carregadas do arquivo." << std::endl;
    return placas;
}

/**
 * @brief Carrega as informações de localização da câmera de um arquivo.
 */
LocalInfo carregarInfoLocalizacao(const std::string& nomeArquivo) {
    LocalInfo info;
    std::ifstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        std::cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de localizacao: " << nomeArquivo << std::endl;
        return info;
    }

    std::string linha;
    while (std::getline(arquivo, linha)) {
        std::stringstream ss(linha);
        std::string chave, valor;
        if (std::getline(ss, chave, ':') && std::getline(ss, valor)) {
            ltrim(valor);
            if (chave == "LATITUDE") info.latitude = valor;
            else if (chave == "LONGITUDE") info.longitude = valor;
            else if (chave == "DESCRICAO") info.local_descricao = valor;
        }
    }
    arquivo.close();
    std::cout << "Localizacao da camera carregada: " << info.local_descricao << std::endl;
    return info;
}


/**
 * @brief Envia um alerta para o Telegram com foto e dados.
 */
void enviarAlertaTelegram(const std::string& placa, const std::string& caminhoImagem, const LocalInfo& localInfo) {
    // --- PREENCHA ESTAS INFORMAÇÕES ---
    const std::string BOT_TOKEN = "8218085317:AAFcx03BlHcmsnWM5SZrPT2prtORemBwRrE"; 
    const std::string CHAT_ID = "5688718831"; // Seu Chat ID
    
    // 1. Obter Horário
    auto agora = std::chrono::system_clock::now();
    auto em_tempo_t = std::chrono::system_clock::to_time_t(agora);
    std::stringstream ss_tempo;
    ss_tempo << std::put_time(std::localtime(&em_tempo_t), "%Y-%m-%d %H:%M:%S");
    std::string horario = ss_tempo.str();

    // 2. Obter Localização
    std::string latitude = localInfo.latitude;
    std::string longitude = localInfo.longitude;
    std::string local_descricao = localInfo.local_descricao;
    std::string google_maps_link = "https://www.google.com/maps?q=" + latitude + "," + longitude;

    // 3. Montar a Mensagem
    std::stringstream ss_msg;
    ss_msg << "🚨 ALERTA DE VEICULO ROUBADO 🚨\n\n"
           << "Placa Detectada: " << placa << "\n"
           << "Horario: " << horario << "\n"
           << "Local: " << local_descricao << "\n"
           << "Localizacao (GPS): " << google_maps_link;
    std::string mensagem = ss_msg.str();

    // --- ENVIO COM LIBCURL ---
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        std::string url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendPhoto";

        curl_mime *form = NULL;
        curl_mimepart *field = NULL;
        form = curl_mime_init(curl);

        // Campo 1: chat_id
        field = curl_mime_addpart(form);
        curl_mime_name(field, "chat_id");
        // *** CORREÇÃO AQUI: Passa (size_t)-1 para o curl medir a string
        curl_mime_data(field, CHAT_ID.c_str(), (size_t)-1);

        // Campo 2: photo (o arquivo)
        field = curl_mime_addpart(form);
        curl_mime_name(field, "photo");
        curl_mime_filedata(field, caminhoImagem.c_str());

        // Campo 3: caption (a mensagem)
        field = curl_mime_addpart(form);
        curl_mime_name(field, "caption");
        // *** CORREÇÃO AQUI: Passa o tamanho real da mensagem
        curl_mime_data(field, mensagem.c_str(), mensagem.length());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        
        // *** NOVA LINHA: Adiciona o callback para "silenciar" a resposta JSON
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "\ncurl_easy_perform() falhou: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "\nAlerta enviado para o Telegram com sucesso!" << std::endl;
        }

        curl_mime_free(form);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Uso: ./meu_alpr <caminho_para_imagem>" << std::endl;
        return 1;
    }

    std::string arquivoDePlacas = "../../PLACAS_ROUBADAS.txt";
    std::vector<std::string> placas_roubadas = carregarPlacasDoArquivo(arquivoDePlacas);

    std::string arquivoLocal = "../../Local_da_Camera.txt";
    LocalInfo localInfo = carregarInfoLocalizacao(arquivoLocal);

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
    std::string placa_roubada_encontrada = "";

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
                    placa_roubada_encontrada = placa_lida;
                    break;
                }
            }
            if (roubada) break;
        }
        if (roubada) break;
    }

    if (roubada) {
        std::cout << "\nACAO: Enviando notificacao as autoridades e ao proprietario!" << std::endl;
        enviarAlertaTelegram(placa_roubada_encontrada, image_path, localInfo);
    } else {
        std::cout << "\nSTATUS: Tudo OK. Nenhuma placa roubada detectada." << std::endl;
    }

    return 0;
}