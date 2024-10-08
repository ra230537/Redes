#!/bin/bash

PORT=8081  # Porta do servidor
IP="127.0.0.1"  # IP do servidor

backlog=12
  echo "Testando com backlog = $backlog"
  ./servidor $PORT $backlog &
  SERVER_PID=$!

  num_clients=15
  for i in $(seq 1 $num_clients); do
    echo "Iniciando cliente $i"
    ./cliente $IP $PORT > cliente_${backlog}_${i}.log 2>&1 &
  done

  sleep 10

  # Monitorar conexões ativas
  netstat -taulpn | grep $PORT

  # Esperar o servidor processar
  sleep 80

  # Encerrar o servidor
  kill $SERVER_PID
  sleep 2