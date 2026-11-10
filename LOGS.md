# Logs do Reprocessing (Android)

As libs `libhl_android_*` (server) e `libclient_android_*` (client) gravam dois arquivos na
pasta de trabalho do Xash, normalmente `/sdcard/xash/` (a mesma do `engine.log`):

- `reprocessing_log.txt`   eventos: lib carregada (com arquitetura e data do build), GameDLLInit, mapa
                 (ServerActivate), jogador entrando, cliente iniciando. Passa de 512 KB e e zerado.
- `reprocessing_crash.log` relatorio de crash: sinal e causa (ex.: "acesso DESALINHADO"), registradores,
                 funcao e modulo da falha (PC), quem chamou (LR), cadeia de chamadas, varredura da pilha
                 e as **ultimas 64 linhas do historico**: ultimas entidades criadas (`spawn monster_...`)
                 e armas pre-carregadas (`precache weapon ...`). Mostra em que ponto do "novo jogo" caiu.

Tambem sai no logcat com as tags `RP` e `RP_CRASH`.

## Se o jogo cair, mande
1. `reprocessing_crash.log`
2. `reprocessing_log.txt`
3. `engine.log` (ja gerado com `-log`)
4. O modelo do aparelho e se e 32 ou 64 bits.

## Resolver enderecos (quem tem as libs com simbolos)
O workflow gera o artefato **Reprocessing-libs-symbols** (libs sem strip, por arquitetura).
Para cada linha `pc 0000abcd libhl_android_armv7l.so`:

    addr2line -f -C -e Reprocessing-libs-symbols/armeabi-v7a/libhl_android_armv7l.so 0x0000abcd

Use a lib da MESMA build que o aparelho rodou (confira a data do build no cabecalho do log).
