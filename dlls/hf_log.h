// Sistema de logs do Reprocessing (Android).
//
//  HF_Log        grava uma linha em hf_log.txt (e no logcat, tag "RP") e guarda no historico.
//  HF_Breadcrumb guarda a linha so em memoria (barato, pode ser chamado a todo momento).
//
// Se o jogo cair (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL), hf_crash.cpp grava em hf_crash.log:
// sinal, endereco, registradores, cadeia de chamadas e as ultimas linhas do historico.
#ifndef HF_LOG_H
#define HF_LOG_H

#if defined( __GNUC__ )
void HF_Log( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
void HF_Breadcrumb( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
#else
void HF_Log( const char *fmt, ... );
void HF_Breadcrumb( const char *fmt, ... );
#endif

// Marcadores permanentes (arquivos vazios na pasta de trabalho do Xash, ex.: /sdcard/xash/).
int HF_FlagExists( const char *name );   // 1 se o marcador existe
void HF_FlagSet( const char *name );     // cria o marcador

#endif // HF_LOG_H
