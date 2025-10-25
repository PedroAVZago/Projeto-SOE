# 🚗 Projeto de Reconhecimento de Placas com OpenALPR

Este repositório contém um projeto completo para reconhecimento de placas de veículos utilizando uma versão customizada e corrigida do OpenALPR. O guia abaixo detalha todo o processo de configuração do ambiente em WSL (Subsistema do Windows para Linux), desde a instalação das dependências até a compilação e execução da aplicação final.

## 📋 Estrutura do Repositório

O projeto está organizado da seguinte maneira:
```
.
├── README.md                <-- Este guia
├── Bibliotecas/
│   └── openalpr/            <-- O código-fonte corrigido do OpenALPR
└── MeuProjetoALPR/
    ├── README.md            <-- Guia específico da sua aplicação
    ├── build/               <-- Pasta para compilação (inicialmente vazia)
    ├── media/
    │   └── carro_teste.jpg  <-- Imagem para testes
    ├── src/
    │   └── main.cpp         <-- O código-fonte da sua aplicação
    └── CMakeLists.txt       <-- O "roteiro" de compilação da sua aplicação
	
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
sudo apt install -y build-essential cmake git pkg-config \libopencv-dev libtesseract-dev \liblog4cplus-dev libcurl4-openssl-dev
sudo apt install libcurl4-openssl-dev -y
```
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

C. 💬 Dependências do Bot Telegram:
- *libcurl4-openssl-dev -y*: Biblioteca para chamadas à API notificação do TELEGRAM.
### 🛠️ Passo 2: Compilando a Biblioteca OpenALPR
Como o OpenALPR não está mais nos repositórios recentes do Ubuntu e requer correções para funcionar com bibliotecas modernas, vamos compilá-lo a partir do código-fonte que está na pasta **Bibliotecas/**.

#### 2.1 - 📁 Navegue até a Pasta do Código-Fonte
Assumindo que você está na raiz do repositório clonado:

**🖥️ Linguagem: Bash**
```Bash
cd Bibliotecas/openalpr/src/
```

#### 2.2 - 📂 Crie a Pasta de Build
É uma boa prática compilar projetos em uma pasta separada para não sujar o diretório do código-fonte.

**🖥️ Linguagem: Bash**
```Bash
mkdir build
cd build
```



#### 2.3 - ⚡ Configure com CMake e Compile com Make
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
#### 2.4 - ✅ Verificação
Se tudo correu bem, o comando alpr agora deve estar disponível no seu sistema. Verifique a instalação com:

**🖥️ Linguagem: Bash**
```
alpr --version
```
Você deverá ver a versão do OpenALPR instalada.

### 🚀 Passo 3: Compilando e Executando a Aplicação (MeuProjetoALPR)
Agora que a biblioteca OpenALPR está instalada e pronta para ser usada, vamos compilar a nossa aplicação customizada que a utiliza.

#### 3.1 - 📁 Navegue até a Pasta do Projeto
Volte para a raiz do repositório e entre na pasta MeuProjetoALPR.

**🖥️ Linguagem: Bash**
```Bash
# Se você ainda está em 'openalpr/src/build', volte 4 níveis:
cd ../../../../MeuProjetoALPR/
```
#### 3.2 - 📂 Crie a Pasta de Build
Assim como antes, vamos usar uma pasta build dedicada.

**🖥️ Linguagem: Bash**
```Bash
cd build
```

#### 3.3 - ⚡ Compile o Projeto
O CMakeLists.txt deste projeto já está configurado para encontrar e usar a biblioteca OpenALPR que acabamos de instalar.

**🖥️ Linguagem: Bash**
```Bash
# 1. Configura o projeto
cmake ..
# 2. Compila a sua aplicação (será bem rápido)
make
```
Se a compilação terminar sem erros, um executável chamado meu_alpr terá sido criado dentro da pasta build.


#### 3.4 - 🎯 Execute!
Para rodar a aplicação, execute o programa a partir da pasta build, passando o caminho da imagem de teste como argumento.

**🖥️ Linguagem: Bash**
```Bash
./meu_alpr ../media/foto_placa1.jpeg
```

📊 Saída Esperada:
```Bash
Analisando Placa 1...
  - Tentativa 1: GZN1B34 (Confianca: 92.541%)
>>> ALERTA: Placa 'GZN1B34' encontrada na lista de roubados!

AÇÃO: Enviando notificacao as autoridades e ao proprietario!
```
