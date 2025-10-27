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
#include "json.hpp" // Biblioteca nlohmann/json (coloque o .hpp aqui)

using json = nlohmann::json;
using namespace std; // Adicionado para simplificar (opcional)

// --- ESTRUTURA PARA ARMAZENAR DADOS DE LOCALIZAÇÃO ---
struct LocalInfo {
    string latitude = "NAO_DEFINIDO";
    string longitude = "NAO_DEFINIDO";
    string local_descricao = "Local Desconhecido";
};

// --- FUNÇÕES DE CALLBACK PARA LIBCURL ---

// Callback para "engolir" a resposta do Telegram (evita JSON no console)
static size_t WriteTelegramCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return size * nmemb;
}

// Callback para capturar a resposta da API de consulta de placa
static size_t WriteApiCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// --- FUNÇÕES AUXILIARES ---

static inline void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
}

bool validaFormatoPlaca(const string& placa) {
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

vector<string> carregarPlacasDoArquivo(const string& nomeArquivo) {
    vector<string> placas;
    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de placas: " << nomeArquivo << endl;
        return placas;
    }
    string linha;
    while (getline(arquivo, linha)) {
        if (!linha.empty()) {
            placas.push_back(linha);
        }
    }
    arquivo.close();
    cout << placas.size() << " placas carregadas do arquivo." << endl;
    return placas;
}

LocalInfo carregarInfoLocalizacao(const string& nomeArquivo) {
    LocalInfo info;
    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo de localizacao: " << nomeArquivo << endl;
        return info;
    }
    string linha;
    while (getline(arquivo, linha)) {
        stringstream ss(linha);
        string chave, valor;
        if (getline(ss, chave, ':') && getline(ss, valor)) {
            ltrim(valor);
            if (chave == "LATITUDE") info.latitude = valor;
            else if (chave == "LONGITUDE") info.longitude = valor;
            else if (chave == "DESCRICAO") info.local_descricao = valor;
        }
    }
    arquivo.close();
    cout << "Localizacao da camera carregada: " << info.local_descricao << endl;
    return info;
}

// --- FUNÇÃO PARA CONECTAR À API DE CONSULTA ---
json api_conexao(const string& placa) {
    string url_base = "https://wdapi2.com.br/consulta";
    string token = "108c6f4dd016b026db32a1863192bc85";
    string url = url_base + "/" + placa + "/" + token;

    CURL* curl;
    CURLcode res;
    string readBuffer; // Buffer para guardar a resposta JSON

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Timeout de 10 segundos
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteApiCallback); // Usa o callback da API
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // Garante que o cacert.pem está na pasta build/
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            cout << "\nConsulta a API realizada com sucesso para placa: " << placa << endl;
            try {
                // Tenta interpretar a resposta como JSON
                return json::parse(readBuffer);
            } catch (json::parse_error& e) {
                cerr << "Erro ao interpretar JSON da API: " << e.what() << endl;
                cerr << "Resposta recebida: " << readBuffer << endl;
            }
        } else {
            cerr << "Erro ao conectar à API de consulta: " << curl_easy_strerror(res) << endl;
        }
    }
    // Se chegou aqui, algo deu errado
    cerr << "Atenção! Nao foi possivel obter dados da API para a placa " << placa << "." << endl;
    return json(); // Retorna um objeto JSON vazio para indicar falha
}


// --- FUNÇÃO PARA ENVIAR ALERTA TELEGRAM (MODIFICADA) ---
void enviarAlertaTelegram(const string& placa, const string& caminhoImagem, const LocalInfo& localInfo, const json& apiData) {
    // --- PREENCHA ESTAS INFORMAÇÕES ---
    const string BOT_TOKEN = "8218085317:AAFcx03BlHcmsnWM5SZrPT2prtORemBwRrE"; // Use o token do seu bot
    const string CHAT_ID = "5688718831";

    // 1. Obter Horário
    auto agora = chrono::system_clock::now();
    auto em_tempo_t = chrono::system_clock::to_time_t(agora);
    stringstream ss_tempo;
    ss_tempo << put_time(localtime(&em_tempo_t), "%Y-%m-%d %H:%M:%S");
    string horario = ss_tempo.str();

    // 2. Obter Localização
    string latitude = localInfo.latitude;
    string longitude = localInfo.longitude;
    string local_descricao = localInfo.local_descricao;
    string google_maps_link = "https://www.google.com/maps?q=" + latitude + "," + longitude;

    // --- 3. EXTRAIR DADOS DA API (com valores padrão) ---
    string marca = apiData.value("MARCA", "N/A");
    string modelo = apiData.value("MODELO", "N/A");
    string cor = apiData.value("cor", "N/A");
    string ano = apiData.value("ano", "N/A");
    string anoModelo = apiData.value("ano_modelo", "N/A");
    string situacao = apiData.value("situacao", "N/A");
    string chassi = apiData.value("chassi", "N/A"); // Últimos 4 dígitos geralmente

    // Tratamento das restrições (igual ao seu exemplo)
    string r1 = apiData.contains("extra") ? apiData["extra"].value("restricao_1", "SEM RESTRICAO") : "SEM RESTRICAO";
    string r2 = apiData.contains("extra") ? apiData["extra"].value("restricao_2", "SEM RESTRICAO") : "SEM RESTRICAO";
    string r3 = apiData.contains("extra") ? apiData["extra"].value("restricao_3", "SEM RESTRICAO") : "SEM RESTRICAO";
    string r4 = apiData.contains("extra") ? apiData["extra"].value("restricao_4", "SEM RESTRICAO") : "SEM RESTRICAO";
    vector<string> restricoes = {r1, r2, r3, r4};
    vector<string> restricoes_validas;
    for (const auto& r : restricoes) if (r != "SEM RESTRICAO") restricoes_validas.push_back(r);
    string resultado_restricoes;
    if (restricoes_validas.empty()) {
        resultado_restricoes = "Nenhuma";
    } else {
        for (size_t i = 0; i < restricoes_validas.size(); ++i) {
            resultado_restricoes += restricoes_validas[i];
            if (i < restricoes_validas.size() - 1) resultado_restricoes += ", ";
        }
    }

    // --- 4. Montar a Mensagem Completa ---
    stringstream ss_msg;
    ss_msg << "🚨 ALERTA DE VEICULO ROUBADO 🚨\n\n"
           << "Placa Detectada: " << placa << "\n"
           << "Horario: " << horario << "\n"
           << "Local: " << local_descricao << "\n"
           << "Localizacao (GPS): " << google_maps_link << "\n\n"
           << "--- Dados do Veiculo (Consulta API) ---\n"
           << "Marca/Modelo: " << marca << " / " << modelo << "\n"
           << "Cor: " << cor << "\n"
           << "Ano Fab/Mod: " << ano << "/" << anoModelo << "\n"
           << "Chassi (Final): " << chassi << "\n"
           << "Situacao: " << situacao << "\n"
           << "Restricoes: " << resultado_restricoes;
    string mensagem = ss_msg.str();

    // --- 5. ENVIO COM LIBCURL (Mesmo de antes, mas com WriteTelegramCallback) ---
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        string url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendPhoto";
        curl_mime *form = curl_mime_init(curl);
        curl_mimepart *field;

        // chat_id
        field = curl_mime_addpart(form);
        curl_mime_name(field, "chat_id");
        curl_mime_data(field, CHAT_ID.c_str(), (size_t)-1);

        // photo
        field = curl_mime_addpart(form);
        curl_mime_name(field, "photo");
        curl_mime_filedata(field, caminhoImagem.c_str());

        // caption
        field = curl_mime_addpart(form);
        curl_mime_name(field, "caption");
        curl_mime_data(field, mensagem.c_str(), mensagem.length());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteTelegramCallback); // Usa o callback do Telegram

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "\ncurl_easy_perform() falhou (Telegram): " << curl_easy_strerror(res) << endl;
        } else {
            cout << "\nAlerta enviado para o Telegram com sucesso!" << endl;
        }
        curl_mime_free(form);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}


// --- FUNÇÃO MAIN MODIFICADA ---
int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Uso: ./meu_alpr <caminho_para_imagem>" << endl;
        return 1;
    }

    string arquivoDePlacas = "../../PLACAS_ROUBADAS.txt";
    vector<string> placas_roubadas = carregarPlacasDoArquivo(arquivoDePlacas);

    string arquivoLocal = "../../Local_da_Camera.txt";
    LocalInfo localInfo = carregarInfoLocalizacao(arquivoLocal);

    if (placas_roubadas.empty()) {
        cerr << "A lista de placas esta vazia ou o arquivo nao foi encontrado. Encerrando." << endl;
        return 1;
    }

    string image_path = argv[1];
    alpr::Alpr openalpr("eu", "/etc/openalpr/openalpr.conf");
    openalpr.setTopN(5);

    if (!openalpr.isLoaded()) {
        cerr << "Erro ao carregar a biblioteca OpenALPR" << endl;
        return 1;
    }

    alpr::AlprResults results = openalpr.recognize(image_path);
    bool roubada_detectada = false;
    string placa_roubada_encontrada = "";
    json dados_api_veiculo = json(); // Objeto JSON para guardar dados da API

    for (int i = 0; i < results.plates.size(); i++) {
        alpr::AlprPlateResult plate = results.plates[i];
        cout << "\nAnalisando Placa " << i+1 << "..." << endl;

        for (int k = 0; k < plate.topNPlates.size(); k++) {
            alpr::AlprPlate candidate = plate.topNPlates[k];
            string placa_lida = candidate.characters;
            float confianca = candidate.overall_confidence;

            cout << "  - Tentativa " << k+1 << ": " << placa_lida
                      << " (Confianca: " << confianca << "%)" << endl;

            if (!validaFormatoPlaca(placa_lida)) {
                cout << "      -> Formato invalido, ignorando." << endl;
                continue;
            }
            if (confianca < 60.0) {
                cout << "      -> Confianca muito baixa, ignorando." << endl;
                continue;
            }

            // Verifica se a placa está na lista de roubadas
            for (const string& placa_roubada : placas_roubadas) {
                if (placa_lida == placa_roubada) {
                    cout << ">>> ALERTA: Placa '" << placa_roubada << "' encontrada na lista de roubados!" << endl;
                    roubada_detectada = true;
                    placa_roubada_encontrada = placa_lida;

                    // --- CHAMADA DA API AQUI ---
                    cout << "Consultando API para detalhes do veiculo..." << endl;
                    dados_api_veiculo = api_conexao(placa_lida); // Chama a API

                    // Mesmo se a API falhar (dados_api_veiculo ficar vazio),
                    // o alerta será enviado, mas com "N/A" nos campos da API.
                    break; // Sai do loop de placas roubadas
                }
            }
            if (roubada_detectada) break; // Sai do loop de candidatas (topNPlates)
        }
        if (roubada_detectada) break; // Sai do loop de placas físicas
    }

    // --- TOMADA DE DECISÃO FINAL ---
    if (roubada_detectada) {
        cout << "\nACAO: Enviando notificacao as autoridades e ao proprietario!" << endl;
        // Chama a função de alerta, passando os dados da API
        enviarAlertaTelegram(placa_roubada_encontrada, image_path, localInfo, dados_api_veiculo);
    } else {
        cout << "\nSTATUS: Tudo OK. Nenhuma placa roubada detectada." << endl;
    }

    return 0;
}