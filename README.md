Sistemas Operacionais "A"
Trabalho Final: Agendamento de execução de tarefas no Linux

Alunos: Gustavo Karkow e Rodrigo Dias de Moura

Especificação da implementação:

# Estruturas:
* Estrutura Task:
    Contém todas as propriedades da tarefa (hh, mm, n, arq_tarefa, incr).

* Estrutura SHM: referencia o segmento de memória compartilhado, contendo
    Vetor de tarefas;
    Contador que gera o id das tarefas;
    Contador de tarefas agendadas;

# IPC:
* Memória compartilhada para comunicação entre processos;
* Semáforos para controle de acesso;
* Set com 3 semáforos:
    Semáforo 0: controle sobre a memória compartilhada (livre/em uso; 0/1)
    Semáforo 1: status do daemon roda_tarefa (acordar/dormir; 0/1)
    Semáforo 2: controle sobre o loop do roda_tarefa (interromper/ativar; 0/1)