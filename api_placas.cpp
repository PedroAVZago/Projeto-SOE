#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"  // Biblioteca nlohmann/json

using json = nlohmann::json;
using namespace std;

// Função auxiliar para capturar o retorno da API (callback do libcurl)
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Função para realizar a conexão com a API
json api_conexao(const string& placa) {
    string url_base = "https://wdapi2.com.br/consulta";
    string token = "108c6f4dd016b026db32a1863192bc85";
    string url = url_base + "/" + placa + "/" + token;

    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            try {
                return json::parse(readBuffer);
            } catch (...) {
                cerr << "Erro ao interpretar JSON." << endl;
            }
        } else {
            cerr << "Erro ao conectar à API: " << curl_easy_strerror(res) << endl;
        }
    }
    cerr << "Atenção! A placa " << placa << " não foi encontrada na base de dados, abordar o veículo." << endl;
    return json(); // Retorna vazio
}

int main() {
    string placa = "PBT5340";

    json dados = api_conexao(placa);
    if (dados.empty()) return 1;

    // Extração das principais informações
    string marca = dados.value("MARCA", "N/A");
    string modelo = dados.value("MODELO", "N/A");
    string cor = dados.value("cor", "N/A");
    string chassi = dados.value("chassi", "N/A");
    string uf = dados.value("uf", "N/A");
    string situacao = dados.value("situacao", "N/A");

    // Restrições
    string r1 = dados["extra"].value("restricao_1", "SEM RESTRICAO");
    string r2 = dados["extra"].value("restricao_2", "SEM RESTRICAO");
    string r3 = dados["extra"].value("restricao_3", "SEM RESTRICAO");
    string r4 = dados["extra"].value("restricao_4", "SEM RESTRICAO");

    vector<string> restricoes = {r1, r2, r3, r4};
    vector<string> restricoes_validas;

    for (const auto& r : restricoes)
        if (r != "SEM RESTRICAO") restricoes_validas.push_back(r);

    string resultado;
    if (restricoes_validas.empty()) {
        resultado = "SEM RESTRICAO";
    } else {
        for (size_t i = 0; i < restricoes_validas.size(); ++i) {
            resultado += restricoes_validas[i];
            if (i < restricoes_validas.size() - 1)
                resultado += ", ";
        }
    }

    // Exibição formatada
    cout << "\nPrincipais informações:\n\n"
         << "Carro modelo: " << marca << " " << modelo << endl
         << "Placa: " << placa << endl
         << "Cor: " << cor << endl
         << "UF: " << uf << endl
         << "Chassi: " << chassi << endl
         << "Restrições: " << resultado << endl
         << "Situação: " << situacao << endl;

    return 0;
}



