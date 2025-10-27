# 🚗 Projeto de Reconhecimento de Placas com OpenALPR

Este repositório contém um projeto completo para reconhecimento de placas de veículos utilizando uma versão customizada e corrigida do OpenALPR. A aplicação identifica placas em imagens, consulta uma API externa (`wdapi2.com.br`) para obter dados do veículo e, caso a placa esteja em uma lista de restrição (ex: roubados), envia um alerta detalhado para o Telegram com a foto e os dados consultados.

*(Este README foca na versão `MeuProjetoALPR` que processa imagens estáticas. Para a versão em tempo real com webcam para Raspberry Pi, veja o diretório `MeuProjetoALPR_RASP/`)*.

## 📋 Estrutura do Repositório

O projeto está organizado da seguinte maneira:
```
.
├── README.md                <-- Este guia
├── Bibliotecas/
│   └── openalpr/            <-- O código-fonte corrigido do OpenALPR
├── MeuProjetoALPR/
    ├── README.md            <-- Guia específico da sua aplicação
    ├── build/ <-- Pasta para compilação (necessita cacert.pem aqui!)
    ├── media/
    │   └── carro_teste.jpg  <-- Imagem para testes
    ├── src/
    │   └── main.cpp         <-- O código-fonte da sua aplicação
    │   └── json.hpp         <-- Biblioteca nlohmann/json (NECESSÁRIO AQUI)
    └── CMakeLists.txt       <-- O "roteiro" de compilação da sua aplicação
├── Local_da_Camera.txt 	 <-- Configuração de localização (na raiz)	
├── PLACAS_ROUBADAS.txt 	 <-- Lista de placas (na raiz)
├── MeuProjetoALPR_RASP/ 	 <-- Código para a RASP
    ├── ...          
```



## 🚀 Utilização do Repositório

### ⚙️ Passo 1: Configuração do Ambiente WSL
Este guia assume que você já tem o WSL2 instalado com uma distribuição Ubuntu.

#### 1.1 - 🔄 Atualização do Sistema
Primeiro, vamos garantir que todos os pacotes do seu sistema estão atualizados. Abra o terminal do WSL e execute:

**🖥️ Linguagem: Bash**
```Bash

sudo apt update && sudo apt upgrade -y

```

#### 1.2 - 📦 Instalação das Dependências Essenciais
Para compilar o OpenALPR e o nosso projeto, precisamos de várias bibliotecas e ferramentas. Este comando instalará tudo de uma vez.

**🖥️ Linguagem: Bash**
```Bash
sudo apt install -y build-essential cmake git pkg-config \
                    libopencv-dev libtesseract-dev \
                    liblog4cplus-dev libcurl4-openssl-dev
```
Este projeto usa a biblioteca nlohmann/json para processar a resposta da API de consulta.
Esta etapa já foi feita, caso não haja arquivo no src, realize-a:

1. Baixe o arquivo json.hpp mais **recente**: Vá para https://github.com/nlohmann/json/releases, encontre a última release e baixe o arquivo json.hpp (geralmente listado nos "Assets").
2. Copie o arquivo json.hpp para dentro da pasta MeuProjetoALPR/src/.
##### 🎯 O que estamos instalando?
A. 🛠️ Ferramentas de Compilação:

- *build-essential*: Pacote fundamental que inclui o compilador C++ (g++) e a ferramenta make.
- *cmake*: Gerenciador do processo de compilação.
- *git*: Para clonar o repositório.
- *pkg-config*: Ajuda o cmake a encontrar as bibliotecas instaladas.

B. 📚 Dependências do OpenALPR:

- *libopencv-dev*: Biblioteca de Visão Computacional (essencial para processamento de imagem).
- *libtesseract-dev*: Biblioteca de OCR (para "ler" os caracteres da placa).
- *liblog4cplus-dev*: Usada pelo OpenALPR para registrar logs.
- *libcurl4-openssl-dev*: Usada para funcionalidades de rede.

C. 💬 Dependências do Bot Telegram e consulta a API:
- *libcurl4-openssl-dev -y*: Biblioteca para chamadas à API notificação do TELEGRAM.
- *nlohmann/json*: Processamento de Resposta API

### ⚙️ Passo 2: Configuração Pré-Compilação (IMPORTANTE!)
Antes de compilar e executar MeuProjetoALPR, alguns arquivos e tokens precisam ser configurados:
1. **Certificado SSL (cacert.pem)**:
	- Para que a chamada à API de consulta (wdapi2.com.br) seja segura (HTTPS), precisamos de um arquivo de certificados CA.
	- Baixe o cacert.pem mais recente em: https://curl.se/docs/caextract.html
	- **CRÍTICO:** Coloque este arquivo cacert.pem dentro da pasta MeuProjetoALPR/build/. Você precisará fazer isso antes de executar o programa (e novamente se limpar a pasta build).
2. Arquivos de Dados (na Raiz PROJETO_SOE):
	-ertifique-se de que os arquivos PLACAS_ROUBADAS.txt (com uma placa por linha) e Local_da_Camera.txt (com DESCRICAO:, LATITUDE:, LONGITUDE:) existem na pasta raiz do projeto (PROJETO_SOE/) e estão preenchidos corretamente.
Como o OpenALPR não está mais nos repositórios recentes do Ubuntu e requer correções para funcionar com bibliotecas modernas, vamos compilá-lo a partir do código-fonte que está na pasta **Bibliotecas/**.

### 🛠️ Passo 3: Compilando a Biblioteca OpenALPR

#### 3.1 - 📁 Navegue até a Pasta do Código-Fonte
Assumindo que você está na raiz do repositório clonado:

**🖥️ Linguagem: Bash**
```Bash
cd Bibliotecas/openalpr/src/
```

#### 3.2 - 📂 Crie a Pasta de Build
É uma boa prática compilar projetos em uma pasta separada para não sujar o diretório do código-fonte.

**🖥️ Linguagem: Bash**
```Bash
mkdir build
cd build
```



#### 3.3 - ⚡ Configure com CMake e Compile com Make
Agora, vamos configurar, compilar e instalar a biblioteca. A compilação (make) pode demorar vários minutos.

**🖥️ Linguagem: Bash**
```Bash
# 1. Configura o projeto:
cmake ..
# 2. Compila o código usando todos os núcleos do seu processador:
make -j$(nproc)
# 3. Instala a biblioteca no sistema:
sudo make install
# 4. Atualiza o cache de bibliotecas do sistema, para ser acessado como função base:
sudo ldconfig
```
#### 3.4 - ✅ Verificação
Se tudo correu bem, o comando alpr agora deve estar disponível no seu sistema. Verifique a instalação com:

**🖥️ Linguagem: Bash**
```
alpr --version
```
Você deverá ver a versão do OpenALPR instalada.

### 🚀 Passo 4: Compilando e Executando a Aplicação (MeuProjetoALPR)
Agora que a biblioteca OpenALPR está instalada e pronta para ser usada, vamos compilar a nossa aplicação customizada que a utiliza.

#### 4.1 - 📁 Navegue até a Pasta do Projeto
Volte para a raiz do repositório e entre na pasta MeuProjetoALPR.

**🖥️ Linguagem: Bash**
```Bash
# Se você ainda está em 'openalpr/src/build', volte 4 níveis:
cd ../../../../MeuProjetoALPR/
```
#### 4.2 - 📂 Crie a Pasta de Build
Assim como antes, vamos usar uma pasta build dedicada.

**🖥️ Linguagem: Bash**
```Bash
cd build
```

#### 4.3 - ⚡ Compile o Projeto
O CMakeLists.txt deste projeto já está configurado para encontrar e usar a biblioteca OpenALPR que acabamos de instalar.

**🖥️ Linguagem: Bash**
```Bash
# 1. Configura o projeto
cmake ..
# 2. Compila a sua aplicação (será bem rápido)
make
```
Se a compilação terminar sem erros, um executável chamado meu_alpr terá sido criado dentro da pasta build.


#### 4.4 - 🎯 Execute!
Para rodar a aplicação, execute o programa a partir da pasta build, passando o caminho da imagem de teste como argumento.

**🖥️ Linguagem: Bash**
```Bash
./meu_alpr ../media/foto_placa1.jpeg
```

📊 Saída Esperada:
```Bash
X placas carregadas do arquivo.
Localizacao da camera carregada: Camera 01 - LOC

Analisando Placa 1...
  - Tentativa 1: GZN1B34 (Confianca: 92.541%)
>>> ALERTA: Placa 'GZN1B34' encontrada na lista de roubados!
Consultando API para detalhes do veiculo...
Consulta a API realizada com sucesso para placa: GZN1B34

ACAO: Enviando notificacao as autoridades e ao proprietario!
Alerta enviado para o Telegram com sucesso!
```
