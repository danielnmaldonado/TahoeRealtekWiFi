# Revisão da build55/app47

Projeto e adaptação macOS: MaLd0n.

Duas revisões independentes encerraram os defeitos encontrados no código de
DFS, potência e app. A revisão final conferiu27 inputs congelados e reproduziu
os casos contra os métodos de produção com ASan/UBSan. Relatórios completos
e hashes acompanham a entrega em REVIEWS/ e VERIFICATION.json.

Foram corrigidos e revalidados: whitelists nativas8814 que recusavam DFS; plano
AU rejeitado que alterava teto vigente; SAR B/C negativo truncado; compensação
térmica que anulava o limite; CSA/potência de AP vinculado com SSID oculto;
faixas regionais parciais escondidas no app; reconexão de perfil oculto2,4GHz.

A revisão de potência usou os repros originais e81 casos B/C/10 AU adicionais.
A revisão final usou108 casos de reconexão nativa e os clocks separados de
autorização visível/potência. Nenhum achado P0–P3 permaneceu nos escopos revistos.

Essa revisão não é validação de rádio, EIRP, tempo de evacuação físico, antena,
WPA2/DHCP/tráfego ou todas as placas/revisões. Não foi carregada a build55 pelo
agente. Esses limites permanecem em VALIDATION-0.14.8.md e DFS-STATION.md.
