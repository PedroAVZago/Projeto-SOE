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

// --- ADICIONADO PARA A VERSÃO DA RASPBERRY PI ---
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

// --- ESTRUTURA PARA ARMAZENAR DADOS DE LOCALIZAÇÃO ---
struct LocalInfo {
    std::string latitude = "NAO_DEFINIDO";
    std::string longitude = "NAO_DEFINIDO";
    std::string local_descricao = "Local Desconhecido";
};

/**
 * @brief Função de callback para "engolir" a resposta do curl.
 */
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
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
 * (Versão corrigida para não cortar a legenda)
 */
void enviarAlertaTelegram(const std::string& placa, const std::string& caminhoImagem, const LocalInfo& localInfo) {
    // --- PREENCHA COM SEU NOVO TOKEN REVOGADO ---
    const std::string BOT_TOKEN = "SEU_TOKEN_AQUI"; 
    const std::string CHAT_ID = "5688718831"; 
    
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

    // --- ENVIO COM LIBCURL (Versão Corrigida) ---
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        std::string url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendPhoto";

        curl_mime *form = NULL;
        curl_mimepart *field = NULL;
        form = curl_mime_init(curl);

        // Campo 1: chat_id (CORRIGIDO)
        field = curl_mime_addpart(form);
        curl_mime_name(field, "chat_id");
        curl_mime_data(field, CHAT_ID.c_str(), (size_t)-1); // -1 para o curl medir a string

        // Campo 2: photo
        field = curl_mime_addpart(form);
        curl_mime_name(field, "photo");
        curl_mime_filedata(field, caminhoImagem.c_str());

        // Campo 3: caption (CORRIGIDO)
        field = curl_mime_addpart(form);
        curl_mime_name(field, "caption");
        curl_mime_data(field, mensagem.c_str(), mensagem.length()); // Passa o tamanho real

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        
        // Silencia a resposta JSON
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


// ###########################################################################
// #
// # FUNÇÃO MAIN REESCRITA PARA RASPBERRY PI E WEBCAM
// #
// ###########################################################################

int main() {
    // --- 1. CARREGAMENTO DOS ARQUIVOS DE CONFIGURAÇÃO ---
    // O executável estará em build/, então voltamos ../../ para a raiz do PROJETO_SOE
    std::string arquivoDePlacas = "../../PLACAS_ROUBADAS.txt";
    std::vector<std::string> placas_roubadas = carregarPlacasDoArquivo(arquivoDePlacas);

    std::string arquivoLocal = "../../Local_da_Camera.txt";
    LocalInfo localInfo = carregarInfoLocalizacao(arquivoLocal);

    if (placas_roubadas.empty()) {
        std::cerr << "Lista de placas vazia ou arquivo nao encontrado. Encerrando." << std::endl;
        return -1;
    }

    // --- 2. INICIALIZAÇÃO DO ALPR E DA WEBCAM ---
    alpr::Alpr openalpr("eu", "/etc/openalpr/openalpr.conf");
    openalpr.setTopN(5);

    // Tenta abrir a câmera padrão (0). Se falhar, tenta a 1, etc.
    cv::VideoCapture cap(0); 
    if (!cap.isOpened()) {
        std::cerr << "Erro: Nao foi possivel abrir a camera 0. Tentando camera 1..." << std::endl;
        cap.open(1);
        if (!cap.isOpened()) {
            std::cerr << "Erro: Nao foi possivel abrir nenhuma camera. Verifique se esta conectada." << std::endl;
            return -1;
        }
    }
    
    if (!openalpr.isLoaded()) {
        std::cerr << "Erro ao carregar a biblioteca OpenALPR" << std::endl;
        return -1;
    }

    std::cout << "\nIniciando monitoramento... Pressione 'q' na janela de video para sair." << std::endl;

    cv::Mat frame;
    int frame_counter = 0;
    const int PROCESS_EVERY_N_FRAMES = 5; // Otimização para a Pi: processa 1 a cada 5 frames
    const std::string TEMP_ALERT_IMAGE_PATH = "/tmp/alpr_alert.jpg"; // Local para salvar a foto
    std::string ultima_placa_roubada = ""; // Cooldown simples para evitar spam

    // --- 3. LOOP PRINCIPAL DE MONITORAMENTO ---
    while (true) {
        cap.read(frame); // Captura um novo frame
        if (frame.empty()) {
            std::cerr << "Erro: Frame da camera esta vazio." << std::endl;
            break;
        }

        frame_counter++;
        bool processar_este_frame = (frame_counter % PROCESS_EVERY_N_FRAMES == 0);
        bool roubada_neste_frame = false;
        std::string placa_encontrada_agora = "";

        // Só executa o ALPR (que é pesado) no frame selecionado
        if (processar_este_frame) {
            alpr::AlprResults results = openalpr.recognize(frame.data, frame.elemSize(), frame.cols, frame.rows);

            for (int i = 0; i < results.plates.size(); i++) {
                alpr::AlprPlateResult plate = results.plates[i];
                
                // Desenha um retângulo verde em TODAS as placas detectadas
                cv::Point p1(plate.plate_points[0].x, plate.plate_points[0].y);
                cv::Point p2(plate.plate_points[2].x, plate.plate_points[2].y);
                cv::rectangle(frame, p1, p2, cv::Scalar(0, 255, 0), 2);

                for (int k = 0; k < plate.topNPlates.size(); k++) {
                    alpr::AlprPlate candidate = plate.topNPlates[k];
                    std::string placa_lida = candidate.characters;
                    float confianca = candidate.overall_confidence;

                    // Aplica nossos filtros de qualidade
                    if (!validaFormatoPlaca(placa_lida)) continue;
                    if (confianca < 60.0) continue;

                    // Compara com a lista de roubados
                    for (const std::string& placa_roubada : placas_roubadas) {
                        if (placa_lida == placa_roubada) {
                            std::cout << ">>> ALERTA: Placa '" << placa_roubada << "' detectada!" << std::endl;
                            roubada_neste_frame = true;
                            placa_encontrada_agora = placa_lida;
                            
                            // Muda a cor do retângulo para VERMELHO
                            cv::rectangle(frame, p1, p2, cv::Scalar(0, 0, 255), 3);
                            break;
                        }
                    }
                    if (roubada_neste_frame) break;
                }
                if (roubada_neste_frame) break;
            }
        } // fim do if(processar_este_frame)

        // --- 4. LÓGICA DE ALERTA ---
        if (roubada_neste_frame && placa_encontrada_agora != ultima_placa_roubada) {
            std::cout << "\nACAO: Enviando alerta para o Telegram..." << std::endl;
            
            // Salva o frame atual como uma imagem temporária
            cv::imwrite(TEMP_ALERT_IMAGE_PATH, frame);

            // Envia o alerta com a imagem salva
            enviarAlertaTelegram(placa_encontrada_agora, TEMP_ALERT_IMAGE_PATH, localInfo);
            
            // Ativa o cooldown para não enviar spam da mesma placa
            ultima_placa_roubada = placa_encontrada_agora; 
        }

        // --- 5. EXIBIÇÃO DE VÍDEO ---
        cv::imshow("Monitoramento ALPR - Pressione 'q' para sair", frame);

        if (cv::waitKey(1) == 'q') {
            break;
        }
    } // fim do while(true)

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
