1. **Script de Teste e Explicação do Funcionamento:**

Para simular entre 5 a 15 clientes tentando se conectar ao servidor ao mesmo tempo, variando o backlog de 0 a 12, podemos criar um script em Bash. O script irá:

- Iniciar o servidor com diferentes valores de backlog.
- Para cada valor de backlog, iniciar múltiplos clientes simultaneamente.
- Utilizar o comando `netstat` para monitorar o número de conexões bem-sucedidas.
- Coletar os dados para posterior análise.

**Script de Teste (`test_script.sh`):**

```bash
#!/bin/bash

# Porta do servidor
SERVER_PORT=5000

# Número máximo de clientes simultâneos
NUM_CLIENTS=15

# Limpar arquivo de log anterior
> server_log.txt

# Array para armazenar os resultados
declare -A results

# Loop sobre os valores de backlog de 0 a 12
for backlog in {0..12}
do
    echo "Iniciando teste com backlog = $backlog"

    # Iniciar o servidor em background
    ./server $SERVER_PORT $backlog &
    SERVER_PID=$!

    # Esperar um momento para garantir que o servidor iniciou
    sleep 2

    # Iniciar os clientes em background
    for ((i=1; i<=NUM_CLIENTS; i++))
    do
        ./client 127.0.0.1 $SERVER_PORT &
        CLIENT_PIDS[$i]=$!
    done

    # Esperar um momento para que as conexões sejam estabelecidas
    sleep 5

    # Usar netstat para contar o número de conexões estabelecidas
    CONNECTIONS=$(netstat -tan | grep ":$SERVER_PORT" | grep ESTABLISHED | wc -l)
    echo "Conexões estabelecidas com backlog $backlog: $CONNECTIONS"

    # Armazenar o resultado
    results[$backlog]=$CONNECTIONS

    # Finalizar os clientes
    for pid in ${CLIENT_PIDS[*]}
    do
        kill -9 $pid 2>/dev/null
    done

    unset CLIENT_PIDS

    # Finalizar o servidor
    kill -9 $SERVER_PID
    wait $SERVER_PID 2>/dev/null

    # Esperar um momento antes do próximo teste
    sleep 2
done

# Salvar os resultados em um arquivo para uso posterior
echo "Backlog,Connections" > results.csv
for backlog in ${!results[@]}
do
    echo "$backlog,${results[$backlog]}" >> results.csv
done

echo "Testes concluídos. Resultados salvos em results.csv."
```

**Explicação do Funcionamento do Script:**

- **SERVER_PORT:** Define a porta na qual o servidor irá escutar. Certifique-se de que essa porta não esteja em uso por outro serviço.
- **NUM_CLIENTS:** Define o número de clientes que serão iniciados em cada teste. Neste caso, usamos 15 clientes para maximizar a carga.
- O script limpa o arquivo de log anterior (`server_log.txt`) para evitar mistura de logs de testes anteriores.
- Usa um loop `for` para variar o valor do backlog de 0 a 12.
- Para cada valor de backlog:
  - Inicia o servidor com o backlog especificado.
  - Inicia os clientes em background, conectando ao servidor.
  - Aguarda alguns segundos para permitir que as conexões sejam estabelecidas.
  - Usa `netstat` para contar o número de conexões estabelecidas ao servidor na porta especificada.
  - Armazena o número de conexões estabelecidas em um array associativo `results`.
  - Finaliza os processos dos clientes e do servidor para limpar antes do próximo teste.
- Após o loop, salva os resultados no arquivo `results.csv`, que será usado pelo script Python para gerar o gráfico.

**Como Utilizar o Script de Teste:**

1. **Permissão de Execução:**

   Certifique-se de que o script tem permissão de execução:

   ```bash
   chmod +x test_script.sh
   ```

2. **Compilar o Servidor e o Cliente:**

   Compile os códigos do servidor e do cliente, gerando os executáveis `server` e `client`:

   ```bash
   gcc -o server server.c -lpthread
   gcc -o client client.c
   ```

3. **Executar o Script de Teste:**

   ```bash
   ./test_script.sh
   ```

4. **Verificar os Resultados:**

   Após a execução, o script exibirá o número de conexões estabelecidas para cada valor de backlog e salvará os resultados em `results.csv`.

---

2. **Porta do Servidor e Como Descobri-la:**

No script de teste fornecido, a porta do servidor é definida pela variável `SERVER_PORT`:

```bash
SERVER_PORT=5000
```

Isso significa que o servidor está configurado para escutar na porta `5000`. Você pode alterar esse valor para qualquer porta disponível no seu sistema, desde que atualize também os scripts e os comandos que dependem dessa porta.

**Como Descobrir a Porta do Servidor:**

- **Analisando o Script ou Comando de Execução:**

  O servidor recebe a porta como argumento de linha de comando:

  ```bash
  ./server $SERVER_PORT $backlog
  ```

  Isso indica que a porta é passada como parâmetro ao iniciar o servidor.

- **Usando netstat:**

  Após iniciar o servidor, você pode usar o comando `netstat` para verificar em qual porta ele está escutando:

  ```bash
  netstat -tan | grep LISTEN | grep ":5000"
  ```

  Isso mostrará se o servidor está escutando na porta `5000`.

- **Consultando o Código Fonte:**

  No código do servidor, a porta é lida dos argumentos:

  ```c
  int port = atoi(argv[1]); // Porta do servidor
  ```

  Portanto, a porta é definida quando você inicia o servidor.

---

3. **Script em Python para Gerar os Gráficos:**

Este script Python lê o arquivo `results.csv` gerado pelo script de teste e produz um gráfico que mostra como o ajuste do backlog impacta o número de conexões bem-sucedidas.

**Código do Script Python (`generate_graph.py`):**

```python
import matplotlib.pyplot as plt
import csv

# Listas para armazenar os valores de backlog e conexões
backlogs = []
connections = []

# Ler os dados do arquivo results.csv
with open('results.csv', 'r') as csvfile:
    csvreader = csv.reader(csvfile)
    next(csvreader)  # Pular o cabeçalho
    for row in csvreader:
        backlogs.append(int(row[0]))
        connections.append(int(row[1]))

# Ordenar os dados pelo valor do backlog
sorted_data = sorted(zip(backlogs, connections))
backlogs, connections = zip(*sorted_data)

# Gerar o gráfico
plt.figure(figsize=(10, 6))
plt.plot(backlogs, connections, marker='o', linestyle='-')
plt.title('Impacto do Backlog no Número de Conexões Bem-sucedidas')
plt.xlabel('Backlog')
plt.ylabel('Número de Conexões Bem-sucedidas')
plt.grid(True)
plt.xticks(range(min(backlogs), max(backlogs)+1))
plt.yticks(range(0, max(connections)+1))
plt.savefig('backlog_impact.png')
plt.show()
```

**Explicação do Funcionamento do Script:**

- **Importações Necessárias:**

  - `matplotlib.pyplot` para plotar o gráfico.
  - `csv` para ler os dados do arquivo `results.csv`.

- **Leitura dos Dados:**

  - O script lê o arquivo `results.csv`, ignora o cabeçalho e armazena os valores de backlog e conexões em listas.
  - Os dados são ordenados pelo valor do backlog para garantir que o gráfico seja plotado corretamente.

- **Geração do Gráfico:**
  - Configura o tamanho da figura para melhor visualização.
  - Plota os dados usando `plt.plot()`, com marcadores nos pontos de dados para facilitar a leitura.
  - Define o título, os rótulos dos eixos e adiciona uma grade ao gráfico para melhorar a legibilidade.
  - Define os ticks dos eixos X e Y para mostrar todos os valores possíveis no intervalo dos dados.
  - Salva o gráfico como `backlog_impact.png`.
  - Exibe o gráfico na tela usando `plt.show()`.

**Como Utilizar o Script Python:**

1. **Instalar Dependências:**

   Certifique-se de que o Python 3 e a biblioteca `matplotlib` estão instalados em seu sistema. Se necessário, instale o `matplotlib`:

   ```bash
   pip3 install matplotlib
   ```

2. **Executar o Script:**

   ```bash
   python3 generate_graph.py
   ```

3. **Verificar o Gráfico:**

   - O script irá gerar o gráfico e exibi-lo na tela.
   - O gráfico também será salvo como `backlog_impact.png` no diretório atual.

**Entendendo o Input do Script:**

- **Arquivo `results.csv`:**

  O script espera que o arquivo `results.csv` tenha o seguinte formato:

  ```
  Backlog,Connections
  0,5
  1,7
  2,10
  ...
  12,15
  ```

  Onde:

  - **Backlog:** Valor do backlog utilizado no teste.
  - **Connections:** Número de conexões bem-sucedidas registradas para aquele valor de backlog.

**Personalizando o Script:**

- **Alterar Título e Rótulos:**

  Você pode modificar o título do gráfico e os rótulos dos eixos alterando as strings em:

  ```python
  plt.title('Novo Título')
  plt.xlabel('Novo Rótulo do Eixo X')
  plt.ylabel('Novo Rótulo do Eixo Y')
  ```

- **Modificar o Intervalo dos Eixos:**

  Se desejar ajustar o intervalo dos eixos manualmente, você pode usar:

  ```python
  plt.xlim(min_value, max_value)
  plt.ylim(min_value, max_value)
  ```

---

**Considerações Finais:**

- **Consistência dos Dados:**

  Certifique-se de que os dados no arquivo `results.csv` correspondem aos resultados reais dos testes. Dados inconsistentes podem levar a gráficos enganosos.

- **Execução dos Testes Múltiplas Vezes:**

  Para obter resultados mais confiáveis, considere executar os testes várias vezes e calcular a média dos resultados para cada valor de backlog.

- **Análise dos Resultados:**

  - O gráfico gerado permitirá visualizar a relação entre o backlog e o número de conexões bem-sucedidas.
  - Um backlog baixo pode resultar em menos conexões bem-sucedidas quando há muitos clientes tentando se conectar simultaneamente.
  - Um backlog mais alto permite que o servidor gerencie melhor um grande número de tentativas de conexão simultâneas.

- **Limitações:**

  - Dependendo do sistema operacional e de suas configurações, pode haver limites no número máximo de conexões ou no tamanho máximo do backlog.
  - Certifique-se de que seu ambiente de teste suporta o número de conexões que você está tentando estabelecer.

---

Espero que essas informações e scripts sejam úteis para você executar os testes, coletar os dados e gerar os gráficos necessários para analisar o impacto do backlog no número de conexões bem-sucedidas em seu servidor de autenticação.
