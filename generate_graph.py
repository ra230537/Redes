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
