
# 📸 ALPR com Captura Periódica para Raspberry Pi

Este programa roda na Raspberry Pi, utilizando uma webcam para capturar imagens periodicamente (via `fswebcam`). Cada imagem é analisada pela biblioteca **OpenALPR** para detectar placas de veículos.

**Funcionalidades:**
* Captura uma foto da webcam a cada 10 segundos (intervalo configurável no código).
* Utiliza OpenALPR para identificar placas na imagem capturada.
* Valida o formato da placa (padrão antigo ou Mercosul) e aplica um filtro de confiança.
* Se uma placa válida for detectada, salva a imagem capturada na pasta `../../capturas` (relativo à pasta `build`), nomeando o arquivo com a placa e timestamp (ex: `ABC1D23_20251028_170000.jpg`).
* Verifica se a placa detectada está na lista de restrição `../../PLACAS_ROUBADAS.txt`.
* Se a placa estiver na lista, consulta a API externa `wdapi2.com.br` para obter dados detalhados do veículo.
* Envia um alerta para o Telegram contendo a foto capturada, dados da placa, horário, localização (definida em `../../Local_da_Camera.txt`) e as informações obtidas da API.
* Possui um cooldown para evitar spam de alertas da mesma placa.
* Projetado para ser executado como um serviço `systemd`, iniciando automaticamente com a Raspberry Pi.

## 🧩 Entendendo o Código (`src/main.cpp`)

O arquivo `src/main.cpp` contém a lógica principal da aplicação. Suas funções chave são:

* **`LocalInfo carregarInfoLocalizacao(const string& nomeArquivo)`:**
    * **Propósito:** Lê o arquivo de texto `Local_da_Camera.txt`.
    * **Funcionamento:** Abre o arquivo especificado, lê linha por linha procurando pelas chaves `DESCRICAO:`, `LATITUDE:`, e `LONGITUDE:`. Armazena os valores encontrados em uma estrutura `LocalInfo`.
    * **Retorno:** Um objeto `LocalInfo` contendo os dados de localização. Retorna valores padrão ("NAO_DEFINIDO", "Local Desconhecido") se o arquivo não for encontrado ou as chaves estiverem ausentes.

* **`vector<string> carregarPlacasDoArquivo(const string& nomeArquivo)`:**
    * **Propósito:** Lê o arquivo de texto `PLACAS_ROUBADAS.txt`.
    * **Funcionamento:** Abre o arquivo especificado, lê cada linha e a adiciona a um vetor de strings. Linhas vazias são ignoradas.
    * **Retorno:** Um `std::vector<std::string>` onde cada string representa uma placa da lista. Retorna um vetor vazio se o arquivo não for encontrado.

* **`bool validaFormatoPlaca(const string& placa)`:**
    * **Propósito:** Verifica se uma string de placa detectada corresponde aos formatos brasileiros válidos (antigo `LLLNNNN` ou Mercosul `LLLNLNN`).
    * **Funcionamento:** Checa se a string tem 7 caracteres. Em seguida, verifica caractere por caractere se corresponde a um dos dois padrões (usando `isalpha` e `isdigit`).
    * **Retorno:** `true` se o formato for válido, `false` caso contrário.

* **`json api_conexao(const string& placa)`:**
    * **Propósito:** Consulta a API externa (`wdapi2.com.br`) para obter informações detalhadas do veículo associado à placa fornecida.
    * **Funcionamento:** Monta a URL da API incluindo a placa e o token de autenticação. Usa a biblioteca `libcurl` para fazer uma requisição HTTP GET. Captura a resposta (que deve ser em formato JSON). Usa a biblioteca `nlohmann/json` para interpretar a resposta. Requer o arquivo `cacert.pem` na pasta de execução (`build/`) para validação SSL.
    * **Retorno:** Um objeto `nlohmann::json` contendo os dados do veículo se a consulta for bem-sucedida e o JSON for válido. Retorna um objeto JSON vazio (`json()`) em caso de erro de conexão ou interpretação.

* **`void enviarAlertaTelegram(const string& placa, const string& caminhoImagemSalva, const LocalInfo& localInfo, const json& apiData)`:**
    * **Propósito:** Envia a notificação de alerta para o Telegram, incluindo a imagem capturada e os dados formatados.
    * **Funcionamento:** Obtém o horário atual. Extrai dados do veículo do objeto `apiData` (com valores padrão "N/A" se a consulta à API falhou). Formata uma mensagem de texto contendo a placa, horário, localização (do `localInfo`) e os dados do veículo. Usa `libcurl` para fazer uma requisição HTTP POST para a API de Bots do Telegram (`sendPhoto`), enviando a imagem (`caminhoImagemSalva`) e a mensagem formatada como legenda (`caption`). Requer o `BOT_TOKEN` e `CHAT_ID` configurados no código.
    * **Retorno:** `void` (não retorna valor, mas imprime mensagens de sucesso ou falha no console/log).

* **`int main()`:**
    * **Propósito:** Orquestra todo o fluxo da aplicação.
    * **Funcionamento:**
        1.  Carrega as configurações (`placas_roubadas`, `localInfo`).
        2.  Inicializa a biblioteca OpenALPR.
        3.  Configura e testa o comando `fswebcam` para o índice correto da câmera.
        4.  Entra em um loop infinito (`while(true)`):
            * Chama `system()` para executar `fswebcam` e capturar uma imagem (`/tmp/alpr_capture.jpg`).
            * Verifica se `fswebcam` teve sucesso.
            * Carrega a imagem capturada com `cv::imread`.
            * Verifica se a imagem foi carregada corretamente.
            * Chama `openalpr.recognize()` passando o *caminho* da imagem temporária.
            * Itera sobre as placas detectadas e suas candidatas:
                * Aplica os filtros `validaFormatoPlaca` e de confiança.
                * Se uma placa válida for encontrada:
                    * Salva a imagem na pasta `../../capturas` com nome `PLACA_TIMESTAMP.jpg`.
                    * Verifica se a placa está na lista `placas_roubadas`.
                    * Se estiver roubada (e fora do cooldown):
                        * Chama `api_conexao()` para obter dados do veículo.
                        * Chama `enviarAlertaTelegram()` com todos os dados.
                        * Atualiza o cooldown.
                    * Usa `goto proxima_captura` para pular para a espera.
            * Se nenhuma placa válida foi encontrada no frame, imprime uma mensagem.
            * Espera 10 segundos (`std::this_thread::sleep_for`).
        5.  (O loop só termina se `Ctrl+C` for pressionado).
    * **Retorno:** `0` em caso de encerramento normal (nunca acontece no loop infinito), ou `-1` em caso de erros fatais na inicialização.

## ⚠️ Pré-requisitos Essenciais

**Antes de compilar este projeto**, certifique-se de que os seguintes passos foram concluídos no seu Raspberry Pi (Ubuntu Server ou Raspberry Pi OS Legacy/Bullseye recomendado):

1.  **Sistema Operacional Configurado:** SSH habilitado, usuário criado, sistema atualizado (`sudo apt update && sudo apt upgrade -y`).
2.  **Dependências de Sistema Instaladas:**
    ```bash
    sudo apt install -y build-essential cmake git pkg-config \
                        libopencv-dev libtesseract-dev libleptonica-dev \
                        liblog4cplus-dev libcurl4-openssl-dev fswebcam
    ```
3.  **Biblioteca OpenALPR Compilada e Instalada:** A biblioteca OpenALPR (da pasta `../../Bibliotecas/openalpr`) **precisa ser compilada manualmente** com as devidas correções nos arquivos CMake. Siga as instruções detalhadas no `README.md` principal (`../../README.md`) para compilar e instalar o OpenALPR **ANTES** de prosseguir. Verifique com `alpr --version`.

## ⚙️ Configuração Pré-Compilação (Neste Projeto)

1.  **Biblioteca JSON (`json.hpp`):**
    * Certifique-se de que o arquivo `json.hpp` (da biblioteca nlohmann/json) está presente nesta pasta `src/`. Se não estiver, baixe-o da [página de releases do nlohmann/json](https://github.com/nlohmann/json/releases) e coloque-o aqui.

2.  **Tokens (no Código `src/main.cpp`):**
    * Edite o arquivo `src/main.cpp` (`nano src/main.cpp`).
    * Localize e substitua os placeholders pelos seus tokens reais:
        * `SEU_TOKEN_API_WDAPI`: Token da API `wdapi2.com.br`.
        * `SEU_TOKEN_TELEGRAM`: Token do seu Bot do Telegram.
        * `SEU_CHAT_ID_TELEGRAM`: O Chat ID para onde os alertas serão enviados (geralmente seu ID de usuário).

3.  **Arquivos de Dados (na Raiz `../../`):**
    * Verifique se os arquivos `../../PLACAS_ROUBADAS.txt` (uma placa por linha) e `../../Local_da_Camera.txt` (formato `CHAVE: VALOR` para `DESCRICAO`, `LATITUDE`, `LONGITUDE`) existem na pasta pai (`Projeto-SOE/`) e estão corretamente preenchidos.

4.  **Certificado SSL (`cacert.pem`):**
    * Este arquivo é necessário para a comunicação segura (HTTPS) com a API `wdapi2`.
    * Ele precisa estar presente na pasta `build/` **antes** de executar o programa. Você pode baixá-lo durante o processo de compilação (veja abaixo).

## 🛠️ Compilação

Siga estes passos dentro do diretório `MeuProjetoALPR_RASP`:

1.  **Navegue até a pasta `build`:**
    ```bash
    cd build
    ```
    *(Se a pasta `build` não existir, crie-a: `mkdir build && cd build`)*

2.  **Limpe compilações anteriores (Opcional, mas recomendado):**
    ```bash
    rm -rf *
    ```

3.  **Baixe o `cacert.pem`:**
    ```bash
    wget [https://curl.se/ca/cacert.pem](https://curl.se/ca/cacert.pem) -O cacert.pem
    ```

4.  **Configure com CMake:**
    O CMake vai verificar as dependências (OpenCV, OpenALPR, Curl) e preparar os arquivos de compilação.
    ```bash
    cmake ..
    ```
    *(Verifique se não há erros e se todas as bibliotecas foram encontradas).*

5.  **Compile com Make:**
    ```bash
    make -j$(nproc)
    ```
    *(O `-j$(nproc)` usa todos os núcleos da CPU para acelerar).*

Se a compilação for bem-sucedida, você terá um executável chamado `alpr_captura_foto` dentro da pasta `build`.

## ▶️ Execução

Existem duas formas de executar o programa:

### 1. Execução Manual (Para Testes)

Ótimo para verificar se tudo está funcionando antes de automatizar.

1.  **Certifique-se de que `cacert.pem` está na pasta `build/`.**
2.  **Execute o programa:** (A partir da pasta `build`)
    ```bash
    ./alpr_captura_foto
    ```
3.  **Observe a Saída:** O terminal mostrará o carregamento dos arquivos, a configuração do `fswebcam`, e depois, a cada 10 segundos, tentará capturar, analisar e, se aplicável, salvar a imagem e enviar alertas.
4.  **Para Parar:** Pressione `Ctrl+C`.

### 2. Execução Automática no Boot (via `systemd`)

Esta é a forma recomendada para operação contínua.

1.  **Crie o Arquivo de Serviço:**
    ```bash
    sudo nano /etc/systemd/system/alpr_capture.service
    ```
2.  **Cole o Seguinte Conteúdo:** (Ajuste `User` e os caminhos se seu nome de usuário não for `soe` ou se a estrutura de pastas for diferente).
    ```ini
    [Unit]
    Description=ALPR Periodic Camera Capture Service
    After=network-online.target
    # Espera a rede estar ativa antes de iniciar

    [Service]
    User=soe
    WorkingDirectory=/home/soe/Projeto-SOE/MeuProjetoALPR_RASP/build/
    ExecStart=/home/soe/Projeto-SOE/MeuProjetoALPR_RASP/build/alpr_captura_foto
    Restart=on-failure
    RestartSec=10
    StandardOutput=journal
    StandardError=journal
    # Reinicia o serviço se ele falhar
    # Espera 10 segundos antes de reiniciar
    # Envia a saída do programa para o log do systemd
    # Envia os erros para o log do systemd

    [Install]
    WantedBy=multi-user.target
    # Inicia o serviço no boot normal (modo multi-usuário)
    ```
3.  **Salve e Saia:** `Ctrl+O`, `Enter`, `Ctrl+X`.

4.  **Recarregue, Habilite e Inicie o Serviço:**
    ```bash
    sudo systemctl daemon-reload
    sudo systemctl enable alpr_capture.service
    sudo systemctl start alpr_capture.service
    ```

5.  **Verifique o Status:**
    ```bash
    sudo systemctl status alpr_capture.service
    ```
    *(Deve mostrar `Active: active (running)` em verde).*

6.  **Monitore os Logs:** A saída do programa agora vai para o log do systemd.
    ```bash
    # Ver logs em tempo real
    journalctl -f -u alpr_capture.service
    ```
    ```bash
    # Ver logs anteriores
    journalctl -b -u alpr_capture.service
    ```

7.  **Reinicie a Pi (`sudo reboot`)** para confirmar que o serviço inicia automaticamente.

## 🐛 Solução de Problemas Comuns

* **Serviço Falha ao Iniciar (`status` mostra `failed`):** Verifique os logs com `journalctl -u alpr_capture.service -n 100`. Causas comuns:
    * `cacert.pem` não encontrado na `WorkingDirectory`.
    * Erro ao abrir a câmera (verifique permissões, se `fswebcam` funciona manualmente).
    * Caminhos incorretos no arquivo `.service`.
    * Tokens de API/Telegram incorretos ou faltando.
    * Arquivos `PLACAS_ROUBADAS.txt` ou `Local_da_Camera.txt` não encontrados ou com formato inválido.
* **`fswebcam` Falha:** Verifique se ele está instalado (`sudo apt install fswebcam`). Teste o comando `fswebcam` manualmente no terminal para ver os erros. Verifique o índice da câmera (`-d /dev/videoX`) no `main.cpp`.
* **Erro de Certificado SSL:** Garanta que `cacert.pem` está na pasta `build` e é um arquivo válido/recente.
* **Alerta Telegram Não Chega:** Verifique o `BOT_TOKEN` e `CHAT_ID` no `main.cpp`. Verifique a conexão de rede da Pi. Olhe os logs (`journalctl`) para erros do `curl`.