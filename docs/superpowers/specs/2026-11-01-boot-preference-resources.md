# Persistência após boot e recursos — build58/app50

Pedido: conservar placa/rede escolhidas após reboot com dois USB; verificar recursos e preparar source publicável. O usuário autorizou executar diretamente no source.

## Arquitetura e escolhas

A seleção volátil por RegistryID deve continuar cercando as operações da sessão. A preferência persistente deve usar a identidade física validada e voltar a resolver o RegistryID a cada enumeração; nunca reutilizar o número de outro boot. Restaurar somente correspondência inequívoca, conservar preferência quando a placa demora a aparecer e respeitar seleção manual, pausa de reconexão e Keychain. A rede preferida não deve depender de salvar uma senha.

Alternativas descartadas: persistir RegistryID falha no reboot; selecionar a primeira placa pode conectar hardware errado; forçar ordem global de serviços não corrige a seleção do app. A rota atual do macOS deve ser observada separadamente.

Recursos: medir app/helper reais em repouso; tratar kernel_task como agregado, não consumo exclusivo da kext. Corrigir somente defeitos observados: formatter criado por registro e timer de coexistência retido após liberar WorkLoop. Preservar intervalos de consulta e prazos de rádio já validados.

Publicação: source completo, dependência SDK verificável, testes isolados, sem artefatos gerados/credenciais; manter licenças e proveniência obrigatórias. Resultados físicos se limitam ao hardware presente e estado realmente observado. O pacote não deve alegar homologação universal nem resolver a divergência RX nativa sem prova.
