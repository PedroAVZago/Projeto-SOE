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
#include "json.hpp" // Biblioteca nlohmann/json

// --- ADICIONADO PARA A VERSÃO DA RASPBERRY PI ---
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using json = nlohmann::json;
using namespace std;

// --- ESTRUTURA PARA ARMAZENAR DADOS DE LOCALIZAÇÃO ---
struct LocalInfo {
    string latitude = "NAO_DEFINIDO";
    string longitude = "NAO_DEFINIDO";
    string local_descricao = "Local Desconhecido";
};

// --- FUNÇÕES DE CALLBACK PARA LIBCURL ---
static size_t WriteTelegramCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return size * nmemb; // Apenas consome os dados
}
static size_t WriteApiCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// --- FUNÇÕES AUXILIARES (validação, carregamento, trim) ---
static inline void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) { return !isspace(ch); }));
}
bool validaFormatoPlaca(const string& placa) {
    if (placa.length() != 7) return false;
    bool antigo = isalpha(placa[0]) && isalpha(placa[1]) && isalpha(placa[2]) && isdigit(placa[3]) && isdigit(placa[4]) && isdigit(placa[5]) && isdigit(placa[6]);
    if (antigo) return true;
    bool mercosul = isalpha(placa[0]) && isalpha(placa[1]) && isalpha(placa[2]) && isdigit(placa[3]) && isalpha(placa[4]) && isdigit(placa[5]) && isdigit(placa[6]);
    if (mercosul) return true;
    return false;
}
vector<string> carregarPlacasDoArquivo(const string& nomeArquivo) {
    vector<string> placas;
    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de placas: " << nomeArquivo << endl; return placas;
    }
    string linha; while (getline(arquivo, linha)) { if (!linha.empty()) placas.push_back(linha); }
    arquivo.close(); cout << placas.size() << " placas carregadas do arquivo." << endl; return placas;
}
LocalInfo carregarInfoLocalizacao(const string& nomeArquivo) {
    LocalInfo info; ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de localizacao: " << nomeArquivo << endl; return info;
    }
    string linha; while (getline(arquivo, linha)) {
        stringstream ss(linha); string chave, valor;
        if (getline(ss, chave, ':') && getline(ss, valor)) {
            ltrim(valor);
            if (chave == "LATITUDE") info.latitude = valor;
            else if (chave == "LONGITUDE") info.longitude = valor;
            else if (chave == "DESCRICAO") info.local_descricao = valor;
        }
    }
    arquivo.close(); cout << "Localizacao da camera carregada: " << info.local_descricao << endl; return info;
}

// --- FUNÇÃO PARA CONECTAR À API DE CONSULTA ---
json api_conexao(const string& placa) {
    string url_base = "https://wdapi2.com.br/consulta";
    // !!! SUBSTITUA PELO SEU TOKEN REAL DA API !!!
    string token = "108c6f4dd016b026db32a1863192bc85";
    string url = url_base + "/" + placa + "/" + token;
    CURL* curl; CURLcode res; string readBuffer;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteApiCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem"); // Precisa estar na pasta build/
        res = curl_easy_perform(curl); curl_easy_cleanup(curl);
        if (res == CURLE_OK) {
            cout << "\nConsulta a API realizada com sucesso para placa: " << placa << endl;
            try { return json::parse(readBuffer); }
            catch (json::parse_error& e) {
                cerr << "Erro ao interpretar JSON da API: " << e.what() << "\nResposta: " << readBuffer << endl;
            }
        } else { cerr << "Erro ao conectar à API de consulta: " << curl_easy_strerror(res) << endl; }
    }
    cerr << "Atenção! Nao foi possivel obter dados da API para a placa " << placa << "." << endl; return json();
}

// --- FUNÇÃO PARA ENVIAR ALERTA TELEGRAM (COM DADOS DA API) ---
void enviarAlertaTelegram(const string& placa, const string& caminhoImagem, const LocalInfo& localInfo, const json& apiData) {
    // !!! SUBSTITUA PELO SEU TOKEN DO BOT !!!
    const string BOT_TOKEN = "8218085317:AAFcx03BlHcmsnWM5SZrPT2prtORemBwRrE";
    const string CHAT_ID = "5688718831";

    auto agora = chrono::system_clock::now(); auto em_tempo_t = chrono::system_clock::to_time_t(agora);
    stringstream ss_tempo; ss_tempo << put_time(localtime(&em_tempo_t), "%Y-%m-%d %H:%M:%S"); string horario = ss_tempo.str();
    string latitude = localInfo.latitude; string longitude = localInfo.longitude; string local_descricao = localInfo.local_descricao;
    string google_maps_link = "https://www.google.com/maps?q=" + latitude + "," + longitude;

    // Extrai dados da API
    string marca = apiData.value("MARCA", "N/A"); string modelo = apiData.value("MODELO", "N/A");
    string cor = apiData.value("cor", "N/A"); string ano = apiData.value("ano", "N/A");
    string anoModelo = apiData.value("ano_modelo", "N/A"); string situacao = apiData.value("situacao", "N/A");
    string chassi = apiData.value("chassi", "N/A");
    string r1="NA", r2="NA", r3="NA", r4="NA"; // Inicializa restrições
    if(apiData.contains("extra")){
        r1 = apiData["extra"].value("restricao_1", "SEM RESTRICAO"); r2 = apiData["extra"].value("restricao_2", "SEM RESTRICAO");
        r3 = apiData["extra"].value("restricao_3", "SEM RESTRICAO"); r4 = apiData["extra"].value("restricao_4", "SEM RESTRICAO");
    }
    vector<string> restricoes = {r1, r2, r3, r4}; vector<string> restricoes_validas;
    for(const auto& r : restricoes) if(r != "SEM RESTRICAO" && r != "NA") restricoes_validas.push_back(r);
    string resultado_restricoes = restricoes_validas.empty() ? "Nenhuma" : "";
    if(!restricoes_validas.empty()) { for(size_t i=0; i<restricoes_validas.size(); ++i) { resultado_restricoes += restricoes_validas[i]; if(i < restricoes_validas.size()-1) resultado_restricoes += ", "; } }

    stringstream ss_msg;
    ss_msg << "🚨 ALERTA DE VEICULO ROUBADO 🚨\n\n"
           << "Placa Detectada: " << placa << "\n" << "Horario: " << horario << "\n"
           << "Local: " << local_descricao << "\n" << "Localizacao (GPS): " << google_maps_link << "\n\n"
           << "--- Dados do Veiculo (Consulta API) ---\n" << "Marca/Modelo: " << marca << " / " << modelo << "\n"
           << "Cor: " << cor << "\n" << "Ano Fab/Mod: " << ano << "/" << anoModelo << "\n"
           << "Chassi (Final): " << chassi << "\n" << "Situacao: " << situacao << "\n"
           << "Restricoes: " << resultado_restricoes;
    string mensagem = ss_msg.str();

    CURL* curl; CURLcode res; curl_global_init(CURL_GLOBAL_ALL); curl = curl_easy_init();
    if (curl) {
        string url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendPhoto";
        curl_mime *form = curl_mime_init(curl); curl_mimepart *field;
        field = curl_mime_addpart(form); curl_mime_name(field, "chat_id"); curl_mime_data(field, CHAT_ID.c_str(), (size_t)-1);
        field = curl_mime_addpart(form); curl_mime_name(field, "photo"); curl_mime_filedata(field, caminhoImagem.c_str());
        field = curl_mime_addpart(form); curl_mime_name(field, "caption"); curl_mime_data(field, mensagem.c_str(), mensagem.length());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteTelegramCallback);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) cerr << "\ncurl_easy_perform() falhou (Telegram): " << curl_easy_strerror(res) << endl;
        else cout << "\nAlerta enviado para o Telegram com sucesso!" << endl;
        curl_mime_free(form); curl_easy_cleanup(curl);
    } curl_global_cleanup();
}

// ###########################################################################
// #
// # FUNÇÃO MAIN PARA RASPBERRY PI E WEBCAM
// #
// ###########################################################################

int main() {
    // --- 1. CARREGAMENTO DOS ARQUIVOS DE CONFIGURAÇÃO ---
    string arquivoDePlacas = "../../PLACAS_ROUBADAS.txt";
    vector<string> placas_roubadas = carregarPlacasDoArquivo(arquivoDePlacas);
    string arquivoLocal = "../../Local_da_Camera.txt";
    LocalInfo localInfo = carregarInfoLocalizacao(arquivoLocal);
    if (placas_roubadas.empty()) { cerr << "Lista de placas vazia. Encerrando." << endl; return -1; }

    // --- 2. INICIALIZAÇÃO DO ALPR E DA WEBCAM ---
    alpr::Alpr openalpr("eu", "/etc/openalpr/openalpr.conf");
    openalpr.setTopN(5); // Pede até 5 candidatas

    cv::VideoCapture cap(0); // Tenta abrir a câmera padrão
    if (!cap.isOpened()) {
        cerr << "Erro: Nao foi possivel abrir a camera 0. Tentando camera 1..." << endl; cap.open(1);
        if (!cap.isOpened()) { cerr << "Erro: Nao foi possivel abrir nenhuma camera." << endl; return -1; }
    }
    if (!openalpr.isLoaded()) { cerr << "Erro ao carregar a biblioteca OpenALPR" << endl; return -1; }

    cout << "\nIniciando monitoramento... Pressione 'q' na janela de video para sair." << endl;

    cv::Mat frame; // Matriz para guardar o frame da câmera
    int frame_counter = 0;
    const int PROCESS_EVERY_N_FRAMES = 5; // Otimização: Processa 1 a cada 5 frames
    const string TEMP_ALERT_IMAGE_PATH = "/tmp/alpr_alert.jpg"; // Local temporário para salvar a foto do alerta
    string ultima_placa_alertada = ""; // Para evitar spam de alertas da mesma placa
    chrono::steady_clock::time_point ultimo_alerta_tempo; // Para cooldown entre alertas

    // --- 3. LOOP PRINCIPAL DE MONITORAMENTO ---
    while (true) {
        cap.read(frame); // Captura um frame
        if (frame.empty()) { cerr << "Erro: Frame da camera esta vazio." << endl; break; }

        frame_counter++;
        bool processar_este_frame = (frame_counter % PROCESS_EVERY_N_FRAMES == 0);
        bool roubada_neste_frame = false;
        string placa_encontrada_agora = "";
        json dados_api_veiculo = json(); // Guarda dados da API se encontrados

        // Só executa o ALPR no frame selecionado
        if (processar_este_frame) {
            alpr::AlprResults results = openalpr.recognize(frame.data, frame.elemSize(), frame.cols, frame.rows);

            for (int i = 0; i < results.plates.size(); i++) {
                alpr::AlprPlateResult plate = results.plates[i];
                cv::Point p1(plate.plate_points[0].x, plate.plate_points[0].y); // Pontos para desenhar retângulo
                cv::Point p2(plate.plate_points[2].x, plate.plate_points[2].y);
                cv::Scalar cor_retangulo = cv::Scalar(0, 255, 0); // Verde por padrão

                for (int k = 0; k < plate.topNPlates.size(); k++) {
                    alpr::AlprPlate candidate = plate.topNPlates[k];
                    string placa_lida = candidate.characters;
                    float confianca = candidate.overall_confidence;

                    if (!validaFormatoPlaca(placa_lida)) continue;
                    if (confianca < 60.0) continue; // Filtro de confiança (ajuste se necessário)

                    // Verifica se está na lista de roubadas
                    for (const string& placa_roubada : placas_roubadas) {
                        if (placa_lida == placa_roubada) {
                            cout << ">>> ALERTA RASP: Placa '" << placa_roubada << "' detectada! Conf: " << confianca << "%" << endl;
                            roubada_neste_frame = true;
                            placa_encontrada_agora = placa_lida;
                            cor_retangulo = cv::Scalar(0, 0, 255); // Muda para Vermelho
                            break;
                        }
                    }
                    if (roubada_neste_frame) break;
                }
                // Desenha o retângulo (verde ou vermelho)
                cv::rectangle(frame, p1, p2, cor_retangulo, 2);
                if (roubada_neste_frame) break; // Sai do loop de placas se já encontrou uma roubada
            }
        } // fim do if(processar_este_frame)

        // --- 4. LÓGICA DE ALERTA (com cooldown) ---
        if (roubada_neste_frame) {
            auto agora = chrono::steady_clock::now();
            // Verifica se é uma placa diferente OU se já passou tempo suficiente (ex: 60s)
            if (placa_encontrada_agora != ultima_placa_alertada ||
                chrono::duration_cast<chrono::seconds>(agora - ultimo_alerta_tempo).count() > 60)
            {
                cout << "\nACAO: Preparando alerta para o Telegram..." << endl;
                cv::imwrite(TEMP_ALERT_IMAGE_PATH, frame); // Salva o frame atual

                cout << "Consultando API para detalhes do veiculo..." << endl;
                dados_api_veiculo = api_conexao(placa_encontrada_agora); // Chama a API

                enviarAlertaTelegram(placa_encontrada_agora, TEMP_ALERT_IMAGE_PATH, localInfo, dados_api_veiculo);

                ultima_placa_alertada = placa_encontrada_agora; // Atualiza a última placa
                ultimo_alerta_tempo = agora; // Atualiza o tempo do último alerta
            }
        }

        // --- 5. EXIBIÇÃO DE VÍDEO ---
        cv::imshow("Monitoramento ALPR (Raspberry Pi) - 'q' para sair", frame);

        if (cv::waitKey(1) == 'q') {
            break; // Sai do loop se 'q' for pressionado
        }
    } // fim do while(true)

    cout << "Encerrando monitoramento..." << endl;
    cap.release();
    cv::destroyAllWindows();
    return 0;
}