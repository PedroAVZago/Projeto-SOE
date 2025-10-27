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
#include <thread> // Para sleep_for
#include <curl/curl.h>
#include <algorithm>
#include "json.hpp" // Biblioteca nlohmann/json

// --- Para chamar comandos externos ---
#include <cstdlib> // Para system()

// --- Para salvar arquivos e criar diretório ---
#include <sys/stat.h>
#include <sys/types.h>

// --- OpenCV (APENAS para carregar imagem e salvar, não para captura) ---
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp> // Para imread/imwrite

using json = nlohmann::json;
using namespace std;

// --- ESTRUTURA PARA ARMAZENAR DADOS DE LOCALIZAÇÃO ---
struct LocalInfo {
    string latitude = "NAO_DEFINIDO";
    string longitude = "NAO_DEFINIDO";
    string local_descricao = "Local Desconhecido";
};

// --- FUNÇÕES DE CALLBACK PARA LIBCURL ---
static size_t WriteTelegramCallback(void* ptr, size_t size, size_t nmemb, void* userdata) { return size * nmemb; }
static size_t WriteApiCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb; output->append((char*)contents, totalSize); return totalSize;
}

// --- FUNÇÕES AUXILIARES (validação, carregamento, trim) ---
static inline void ltrim(string &s) { s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch){ return !isspace(ch); })); }
bool validaFormatoPlaca(const string& placa) {
    if (placa.length() != 7) return false;
    bool antigo = isalpha(placa[0]) && isalpha(placa[1]) && isalpha(placa[2]) && isdigit(placa[3]) && isdigit(placa[4]) && isdigit(placa[5]) && isdigit(placa[6]);
    if (antigo) return true;
    bool mercosul = isalpha(placa[0]) && isalpha(placa[1]) && isalpha(placa[2]) && isdigit(placa[3]) && isalpha(placa[4]) && isdigit(placa[5]) && isdigit(placa[6]);
    if (mercosul) return true;
    return false;
}
vector<string> carregarPlacasDoArquivo(const string& nomeArquivo) {
    vector<string> placas; ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()){ cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de placas: " << nomeArquivo << endl; return placas; }
    string linha; while (getline(arquivo, linha)){ if (!linha.empty()) placas.push_back(linha); }
    arquivo.close(); cout << placas.size() << " placas carregadas do arquivo." << endl; return placas;
}
LocalInfo carregarInfoLocalizacao(const string& nomeArquivo) {
    LocalInfo info; ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()){ cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de localizacao: " << nomeArquivo << endl; return info; }
    string linha; while (getline(arquivo, linha)){
        stringstream ss(linha); string chave, valor;
        if (getline(ss, chave, ':') && getline(ss, valor)){ ltrim(valor);
            if (chave == "LATITUDE") info.latitude = valor; else if (chave == "LONGITUDE") info.longitude = valor; else if (chave == "DESCRICAO") info.local_descricao = valor;
        }
    }
    arquivo.close(); cout << "Localizacao da camera carregada: " << info.local_descricao << endl; return info;
}
// --- FUNÇÃO PARA CONECTAR À API DE CONSULTA ---
json api_conexao(const string& placa) {
    string url_base = "https://wdapi2.com.br/consulta"; string token = "108c6f4dd016b026db32a1863192bc85"; // !!! SEU TOKEN AQUI !!!
    string url = url_base + "/" + placa + "/" + token; CURL* curl; CURLcode res; string readBuffer;
    curl = curl_easy_init(); if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteApiCallback); curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem"); res = curl_easy_perform(curl); curl_easy_cleanup(curl);
        if (res == CURLE_OK){ cout << "\nConsulta API OK para placa: " << placa << endl; try { return json::parse(readBuffer); }
            catch (json::parse_error& e){ cerr << "Erro JSON API: " << e.what() << "\nResp: " << readBuffer << endl; }
        } else { cerr << "Erro conexao API: " << curl_easy_strerror(res) << endl; }
    } cerr << "Falha ao obter dados API para placa " << placa << "." << endl; return json();
}
// --- FUNÇÃO PARA ENVIAR ALERTA TELEGRAM ---
void enviarAlertaTelegram(const string& placa, const string& caminhoImagemSalva, const LocalInfo& localInfo, const json& apiData) {
    const string BOT_TOKEN = "8218085317:AAFcx03BlHcmsnWM5SZrPT2prtORemBwRrE"; // !!! SEU TOKEN AQUI !!!
    const string CHAT_ID = "5688718831"; // SEU CHAT ID
    auto agora=chrono::system_clock::now(); auto em_tempo_t=chrono::system_clock::to_time_t(agora); stringstream ss_tempo; ss_tempo << put_time(localtime(&em_tempo_t), "%Y-%m-%d_%H-%M-%S"); string horario_fmt = ss_tempo.str(); ss_tempo.str(""); ss_tempo.clear(); ss_tempo << put_time(localtime(&em_tempo_t), "%Y-%m-%d %H:%M:%S"); string horario_msg = ss_tempo.str();
    string latitude = localInfo.latitude; string longitude = localInfo.longitude; string local_descricao = localInfo.local_descricao; string google_maps_link = "https://www.google.com/maps?q=" + latitude + "," + longitude;
    string marca=apiData.value("MARCA","N/A"); string modelo=apiData.value("MODELO","N/A"); string cor=apiData.value("cor","N/A"); string ano=apiData.value("ano","N/A"); string anoModelo=apiData.value("ano_modelo","N/A"); string situacao=apiData.value("situacao","N/A"); string chassi=apiData.value("chassi","N/A");
    string r1="NA",r2="NA",r3="NA",r4="NA"; if(apiData.contains("extra")){ r1=apiData["extra"].value("restricao_1","SR"); r2=apiData["extra"].value("restricao_2","SR"); r3=apiData["extra"].value("restricao_3","SR"); r4=apiData["extra"].value("restricao_4","SR"); } vector<string> rest={r1,r2,r3,r4}; vector<string> rest_v; for(const auto& r:rest) if(r!="SR"&&r!="NA") rest_v.push_back(r); string res_r=rest_v.empty()?"Nenhuma":""; if(!rest_v.empty()){for(size_t i=0;i<rest_v.size();++i){res_r+=rest_v[i]; if(i<rest_v.size()-1)res_r+=", ";}}
    stringstream ss_msg; ss_msg << "🚨 ALERTA VEICULO ROUBADO 🚨\n\nPlaca: " << placa << "\nHorario: " << horario_msg << "\nLocal: " << local_descricao << "\nGPS: " << google_maps_link << "\n\n--- Dados API ---\nMarca/Mod: " << marca << "/" << modelo << "\nCor: " << cor << "\nAno: " << ano << "/" << anoModelo << "\nChassi: " << chassi << "\nSituacao: " << situacao << "\nRestr: " << res_r; string mensagem = ss_msg.str();
    CURL* curl; CURLcode res; curl_global_init(CURL_GLOBAL_ALL); curl = curl_easy_init(); if (curl) {
        string url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendPhoto"; curl_mime *form = curl_mime_init(curl); curl_mimepart *field;
        field=curl_mime_addpart(form); curl_mime_name(field,"chat_id"); curl_mime_data(field, CHAT_ID.c_str(), (size_t)-1);
        field=curl_mime_addpart(form); curl_mime_name(field,"photo"); curl_mime_filedata(field, caminhoImagemSalva.c_str());
        field=curl_mime_addpart(form); curl_mime_name(field,"caption"); curl_mime_data(field, mensagem.c_str(), mensagem.length());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); curl_easy_setopt(curl, CURLOPT_MIMEPOST, form); curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteTelegramCallback);
        res = curl_easy_perform(curl); if(res != CURLE_OK) cerr << "\nFalha Telegram: " << curl_easy_strerror(res) << endl; else cout << "\nAlerta Telegram OK!" << endl;
        curl_mime_free(form); curl_easy_cleanup(curl);
    } curl_global_cleanup();
}

// ###########################################################################
// # FUNÇÃO MAIN - LÓGICA DE CAPTURA PERIÓDICA COM FSWEBCAM
// ###########################################################################
int main() {
    // --- 1. CARREGAMENTO DOS ARQUIVOS DE CONFIGURAÇÃO ---
    string arquivoDePlacas = "../../PLACAS_ROUBADAS.txt"; vector<string> placas_roubadas = carregarPlacasDoArquivo(arquivoDePlacas);
    string arquivoLocal = "../../Local_da_Camera.txt"; LocalInfo localInfo = carregarInfoLocalizacao(arquivoLocal);
    string pastaCapturas = "../../capturas/"; // Caminho relativo da build para a pasta de capturas
    if (placas_roubadas.empty()){ cerr << "Lista de placas vazia. Encerrando." << endl; return -1; }

    // --- 2. INICIALIZAÇÃO DO ALPR ---
    alpr::Alpr openalpr("eu", "/etc/openalpr/openalpr.conf"); openalpr.setTopN(5);
    if (!openalpr.isLoaded()){ cerr << "Erro ao carregar a biblioteca OpenALPR" << endl; return -1; }

    // --- 3. CONFIGURAÇÃO DA CAPTURA COM FSWEBCAM ---
    // !!! AJUSTE O ÍNDICE (-d /dev/videoX) CONFORME O TESTE DO FSWEBCAM !!!
    string device_index = "0"; // Tenta o índice 0 primeiro
    string temp_image_path = "/tmp/alpr_capture.jpg"; // Arquivo temporário para fswebcam
    string fswebcam_cmd_base = "fswebcam -r 640x480 --no-banner -S 15 -d /dev/video"; // Resolução, sem banner, pula 15 frames
    // Tenta construir o comando para índice 1, se falhar, tenta índice 0
    string fswebcam_cmd = fswebcam_cmd_base + device_index + " " + temp_image_path;
    cout << "Testando comando fswebcam para /dev/video" << device_index << "..." << endl;
    int test_result = system((fswebcam_cmd + " > /dev/null 2>&1").c_str()); // Testa silenciosamente
    if (test_result != 0) {
        cerr << "Falha ao testar /dev/video" << device_index << ". Tentando /dev/video0..." << endl;
        device_index = "0";
        fswebcam_cmd = fswebcam_cmd_base + device_index + " " + temp_image_path;
        cout << "Testando comando fswebcam para /dev/video" << device_index << "..." << endl;
        test_result = system((fswebcam_cmd + " > /dev/null 2>&1").c_str());
        if (test_result != 0) {
            cerr << "ERRO FATAL: fswebcam falhou para /dev/video0 e /dev/video1. Verifique se 'fswebcam' esta instalado e a camera funciona." << endl;
            return -1;
        }
    }
    cout << "fswebcam configurado para usar /dev/video" << device_index << endl;


    cout << "\nIniciando captura periodica (1 foto a cada 10s)... Pressione Ctrl+C para sair." << endl;
    cv::Mat frame; // Matriz para carregar a imagem capturada
    string ultima_placa_alertada = ""; chrono::steady_clock::time_point ultimo_alerta_tempo;

    // --- 4. LOOP PRINCIPAL DE CAPTURA E PROCESSAMENTO ---
    while (true) {
        cout << "\nCapturando imagem com fswebcam..." << endl;
        // Chama o fswebcam para capturar a imagem
        int result = system(fswebcam_cmd.c_str());

        if (result != 0) {
            cerr << "ERRO: Falha ao executar fswebcam (codigo: " << result << "). Pulando captura." << endl;
            this_thread::sleep_for(chrono::seconds(10)); // Espera antes de tentar de novo
            continue;
        }

        // Carrega a imagem capturada pelo fswebcam usando OpenCV
        frame = cv::imread(temp_image_path, cv::IMREAD_COLOR);

        if (frame.empty()){
            cerr << "ERRO: Nao foi possivel carregar a imagem capturada '" << temp_image_path << "'. Pulando..." << endl;
            this_thread::sleep_for(chrono::seconds(10)); // Espera antes de tentar de novo
            continue;
        }

        cout << "Imagem capturada e carregada (" << frame.cols << "x" << frame.rows << "). Analisando com ALPR..." << endl;

        alpr::AlprResults results = openalpr.recognize(temp_image_path); // OpenALPR pode ler direto do arquivo
        bool placa_valida_encontrada_neste_frame = false;

        for (int i = 0; i < results.plates.size(); i++) {
            alpr::AlprPlateResult plate = results.plates[i];
            for (int k = 0; k < plate.topNPlates.size(); k++) {
                alpr::AlprPlate candidate = plate.topNPlates[k];
                string placa_lida = candidate.characters;
                float confianca = candidate.overall_confidence;

                cout << "  - Candidata " << k+1 << ": " << placa_lida << " (Conf: " << confianca << "%)" << endl;

                if (!validaFormatoPlaca(placa_lida)) continue;
                if (confianca < 75.0) continue; // Ajuste a confiança se necessário

                // --- PLACA VÁLIDA ENCONTRADA ---
                placa_valida_encontrada_neste_frame = true;
                cout << ">>> Placa valida encontrada: " << placa_lida << endl;

                auto agora_t = chrono::system_clock::to_time_t(chrono::system_clock::now());
                stringstream ss_timestamp; ss_timestamp << put_time(localtime(&agora_t), "%Y%m%d_%H%M%S"); string timestamp = ss_timestamp.str();
                string nome_arquivo_salvo = pastaCapturas + placa_lida + "_" + timestamp + ".jpg";

                // Copia a imagem temporária para a pasta de capturas com o nome correto
                cout << "Salvando imagem em: " << nome_arquivo_salvo << endl;
                try {
                    // Usa filesystem para copiar, mais robusto que system("cp")
                    // Note: Requires C++17 filesystem library, might need adjustments if using older compiler
                    // #include <filesystem>
                    // std::filesystem::copy(temp_image_path, nome_arquivo_salvo);

                    // Alternativa com system("cp") se filesystem não estiver disponível/configurado
                     string copy_cmd = "cp " + temp_image_path + " " + nome_arquivo_salvo;
                     system(copy_cmd.c_str());

                } catch (const std::exception& e) {
                    cerr << "ERRO ao copiar/salvar a imagem: " << e.what() << endl;
                    // Se não conseguiu salvar, usa a imagem temporária para o Telegram
                    nome_arquivo_salvo = temp_image_path;
                }


                // --- VERIFICAR SE É ROUBADA E ENVIAR ALERTA ---
                bool roubada_agora = false;
                for (const string& placa_roubada : placas_roubadas) { if (placa_lida == placa_roubada) { roubada_agora = true; break; } }

                if (roubada_agora) {
                     cout << "!!! PLACA ENCONTRADA NA LISTA DE ROUBADOS !!!" << endl;
                     auto agora = chrono::steady_clock::now();
                     if (placa_lida != ultima_placa_alertada || chrono::duration_cast<chrono::seconds>(agora - ultimo_alerta_tempo).count() > 60) {
                        cout << "Consultando API e enviando alerta Telegram..." << endl;
                        json dados_api = api_conexao(placa_lida);
                        enviarAlertaTelegram(placa_lida, nome_arquivo_salvo, localInfo, dados_api); // Envia a imagem salva
                        ultima_placa_alertada = placa_lida; ultimo_alerta_tempo = agora;
                     } else { cout << "Alerta para esta placa enviado recentemente (cooldown)." << endl; }
                }
                goto proxima_captura; // Já achou uma placa válida, vai para o sleep
            } // Fim loop candidatas
        } // Fim loop placas físicas

    proxima_captura:
        if (!placa_valida_encontrada_neste_frame) { cout << "Nenhuma placa valida encontrada neste frame." << endl; }
        cout << "Aguardando 10 segundos para proxima captura..." << endl;
        this_thread::sleep_for(chrono::seconds(10));

    } // Fim do while(true)

    cout << "Encerrando..." << endl;
    // Não precisa mais de cap.release() ou destroyAllWindows()
    return 0;
}
