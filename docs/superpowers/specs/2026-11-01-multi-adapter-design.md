# Atualização por adaptador e recuperação das famílias USB

Pedido: conferir os dois USB reais, buscar redes automaticamente quando necessário
na troca de placa e aplicar as correções recentes a todos os backends elegíveis.
A execução continua diretamente no source, conforme instrução do usuário.

O app atual muda o adaptador e lê SCAN_RESULTS, sem iniciar necessariamente SCAN.
Usaremos estado de busca por RegistryID: cache sem origem confiável ou vencido
num adaptador selecionado e ocioso exige uma busca real coalescida. Uma associação
ativa não deve ser interrompida por busca automática. Hotplug, wake, troca rápida,
resultados atrasados e falha têm invalidação/limites explícitos. Rádio ainda não
inicializado não significa rádio desabilitado. Sem novas credenciais ou JOIN.

Alternativas consideradas: timer de SCAN global interromperia links e misturaria
placas; somente cache mantém o defeito; política por adaptador e estado atende ao
pedido e conserva os epochs existentes. A escolha é de implementação do agente.

O registro PM e a recuperação RX em operação já são comuns aos 13 backends.
A validação da recepção inicial, drenagem e orçamento de clearStall passa ao mesmo
conjunto, preservando capacidade/tipo e erros originais. UNKNOWN/8814BU não ganham
estação. O retry completo de bootstrap exige stop próprio verificado, drain de
callbacks e identidade imutável. As rotinas AU não serão copiadas para outro chip.
Outras famílias recebem seus próprios stops e reidentificação; o legado requer
power-off primário antes de habilitar retry de erro persistente.

Testes de métodos reais com erro antes da correção, limites de retry, callbacks
fora de ordem, troca de ID/epoch e dois controladores independentes. A entrega
trará source completo, compilações do ZIP e evidência das limitações físicas.
Sem reboot, alteração de EFI, firmware de roteador ou proteções do sistema.
