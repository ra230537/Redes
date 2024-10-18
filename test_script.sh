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
    ./servidor $SERVER_PORT $backlog &
    SERVER_PID=$!

    # Esperar um momento para garantir que o servidor iniciou
    sleep 2

    # Iniciar os clientes em background
    for ((i=1; i<=NUM_CLIENTS; i++))
    do
        ./cliente 127.0.0.1 $SERVER_PORT &
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
