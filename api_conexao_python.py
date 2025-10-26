import requests

#Configuração dos parâmetros da API que são constantes
url_base = "https://wdapi2.com.br/consulta"
token = "108c6f4dd016b026db32a1863192bc85"
# placa = 'RHA0A01'
placa = 'PBT5340'

def api_conexao(placa):
    url = f"{url_base}/{placa}/{token}"
    try:
        #Conexão com a API
        response = requests.get(url, timeout=10)
        response.raise_for_status()  
        dados = response.json()
        retorno = dados
    except:
        retorno = print(f"Atenção! A placa {placa} não foi encontrada na base de dados, abordar o veículo.")
        
    return dados

#Extração das principais informações
dados = api_conexao(placa)

marca = dados['MARCA']
modelo = dados['MODELO']
cor = dados['cor']
chassi = dados['chassi']
r_1 = dados['extra']['restricao_1']
r_2 = dados['extra']['restricao_2']
r_3 = dados['extra']['restricao_3']
r_4 = dados['extra']['restricao_4']
uf = dados['uf']
situacao = dados['situacao']


# Lista de restrições
restricoes = [r_1, r_2, r_3, r_4]
restricoes_validas = [r for r in restricoes if r != "SEM RESTRICAO"]

# Monta o resultado
if not restricoes_validas:
    resultado = "SEM RESTRICAO"
else:
    resultado = ", ".join(restricoes_validas)


texto = f"""
Principais informações:\n
Carro modelo: {marca} {modelo}
Placa: {placa}
Cor: {cor}
UF: {uf}
Chassi: {chassi}
Restrições: {restricoes_validas}
Situação: {situacao}

"""

print(texto)

