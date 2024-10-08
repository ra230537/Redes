#!/bin/bash

PORT=8080  # Porta do servidor
IP="127.0.0.1"  # IP do servidor

for backlog in {0..12}; do
  echo "Testando com backlog = $backlog"
  ./servidor $PORT $backlog &
  SERVER_PID=$!

  num_clients=$((RANDOM % 11 + 5))  # Gera um número aleatório entre 5 e 15
  for i in $(seq 1 $num_clients); do
    echo "Iniciando cliente $i"
    ./cliente $IP $PORT > cliente_${backlog}_${i}.log 2>&1 &
  done

  # Esperar o servidor processar
  sleep 10

  # Monitorar conexões ativas
  netstat -taulpn | grep $PORT

  # Encerrar o servidor
  kill $SERVER_PID
  sleep 2
done
