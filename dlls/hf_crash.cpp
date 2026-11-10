// Registro de logs e de crash para as libs do Reprocessing (client e server).
//
// Arquivos (na pasta de trabalho do Xash, normalmente /sdcard/xash/):
//   reprocessing_log.txt    eventos do jogo (carregamento das libs, mapa, jogador entrando...)
//   reprocessing_crash.log  relatorio de crash: sinal, endereco, registradores, cadeia de chamadas
//                 e as ultimas linhas do historico (ex.: ultimas entidades criadas)
//
// Os enderecos "pc XXXXXXXX modulo" resolvem com a lib COM simbolos (artefato Reprocessing-libs-symbols):
//   addr2line -f -C -e libhl_android_armv7l.so 0xXXXXXXXX
#include "hf_log.h"

#if defined( __unix__ ) || defined( __ANDROID__ ) || defined( __linux__ )

#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <dlfcn.h>
#include <ucontext.h>
#include <sys/stat.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef CLIENT_DLL
#define HF_LIBNAME "client"
#else
#define HF_LIBNAME "server"
#endif

namespace
{
	const int kSignals[] = { SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL };
	const int kNumSignals = sizeof( kSignals ) / sizeof( kSignals[0] );
	const int kMaxChain = 24;
	const int kMaxScan = 24;

	enum { RING_SIZE = 64, RING_LEN = 128 };

	struct sigaction g_old[32];
	char g_altstack[64 * 1024];
	char g_dir[256];                       // pasta onde os arquivos sao gravados (com "/" no fim)
	char g_ring[RING_SIZE][RING_LEN];      // historico em memoria
	volatile unsigned g_ringPos = 0;
	volatile int g_dumped = 0;             // esta lib ja escreveu a parte dela do relatorio
	long g_startSec = 0;

	long NowSec()
	{
		struct timespec ts;
		clock_gettime( CLOCK_MONOTONIC, &ts );
		return (long)ts.tv_sec;
	}

	void LogcatWrite( int prio, const char *tag, const char *line )
	{
#ifdef __ANDROID__
		__android_log_print( prio, tag, "%s", line );
#else
		(void)prio; (void)tag; (void)line;
#endif
	}

	// descobre uma pasta onde da pra gravar
	void FindDir()
	{
		char pkg[128] = { 0 };
		int fd = open( "/proc/self/cmdline", O_RDONLY );
		if( fd >= 0 )
		{
			read( fd, pkg, sizeof( pkg ) - 1 );
			close( fd );
		}

		char cand[6][256];
		snprintf( cand[0], sizeof( cand[0] ), "./" );
		snprintf( cand[1], sizeof( cand[1] ), "/sdcard/xash/" );
		snprintf( cand[2], sizeof( cand[2] ), "/storage/emulated/0/xash/" );
		snprintf( cand[3], sizeof( cand[3] ), "/sdcard/Android/data/%s/files/", pkg );
		snprintf( cand[4], sizeof( cand[4] ), "/sdcard/" );
		snprintf( cand[5], sizeof( cand[5] ), "/data/local/tmp/" );

		for( int i = 0; i < 6; i++ )
		{
			char path[300];
			snprintf( path, sizeof( path ), "%sreprocessing_log.txt", cand[i] );
			int f = open( path, O_WRONLY | O_CREAT | O_APPEND, 0644 );
			if( f >= 0 )
			{
				close( f );
				strncpy( g_dir, cand[i], sizeof( g_dir ) - 1 );
				return;
			}
		}
		g_dir[0] = 0;
	}

	int OpenFile( const char *name )
	{
		if( !g_dir[0] )
			FindDir();
		if( !g_dir[0] )
			return -1;

		char path[300];
		snprintf( path, sizeof( path ), "%s%s", g_dir, name );
		return open( path, O_WRONLY | O_CREAT | O_APPEND, 0644 );
	}

	void Emit( int fd, const char *line )
	{
		if( fd >= 0 )
			write( fd, line, strlen( line ) );
		LogcatWrite( 6 /*ANDROID_LOG_ERROR*/, "RP_CRASH", line );
	}

	uintptr_t Clean( uintptr_t a )
	{
#if defined( __arm__ )
		return a & ~(uintptr_t)1; // bit 0 marca Thumb
#else
		return a;
#endif
	}

	void Describe( int fd, const char *label, uintptr_t addr )
	{
		char line[512];
		Dl_info info;
		memset( &info, 0, sizeof( info ) );
		uintptr_t a = Clean( addr );

		if( a && dladdr( (void *)a, &info ) && info.dli_fname )
		{
			const char *mod = strrchr( info.dli_fname, '/' );
			mod = mod ? mod + 1 : info.dli_fname;
			unsigned long modOff = (unsigned long)( a - (uintptr_t)info.dli_fbase );
			if( info.dli_sname )
				snprintf( line, sizeof( line ), "  %-5s pc %08lx  %s  (%s+%lu)\n", label, modOff, mod,
					info.dli_sname, (unsigned long)( a - (uintptr_t)info.dli_saddr ) );
			else
				snprintf( line, sizeof( line ), "  %-5s pc %08lx  %s\n", label, modOff, mod );
		}
		else
		{
			snprintf( line, sizeof( line ), "  %-5s %p  (fora de qualquer lib / nulo)\n", label, (void *)a );
		}
		Emit( fd, line );
	}

	const char *CodeText( int sig, int code )
	{
		if( sig == SIGSEGV )
		{
			if( code == SEGV_MAPERR ) return "endereco nao mapeado (ponteiro nulo ou invalido)";
			if( code == SEGV_ACCERR ) return "sem permissao para acessar a memoria";
		}
		else if( sig == SIGBUS )
		{
			if( code == BUS_ADRALN ) return "acesso a memoria DESALINHADO";
			if( code == BUS_ADRERR ) return "endereco fisico inexistente";
			if( code == BUS_OBJERR ) return "erro de objeto na memoria";
		}
		else if( sig == SIGFPE )
		{
			if( code == FPE_INTDIV ) return "divisao inteira por zero";
			if( code == FPE_FLTDIV ) return "divisao de ponto flutuante por zero";
		}
		else if( sig == SIGILL )
		{
			if( code == ILL_ILLOPC ) return "instrucao ilegal";
		}
		else if( sig == SIGABRT )
		{
			return "abort() (assert/erro fatal)";
		}
		return "";
	}

	void DumpBreadcrumbs( int fd )
	{
		char line[RING_LEN + 48];
		unsigned pos = g_ringPos;
		unsigned count = pos < (unsigned)RING_SIZE ? pos : (unsigned)RING_SIZE;

		snprintf( line, sizeof( line ), "Historico da lib %s (ultimas %u linhas, a mais nova no fim):\n", HF_LIBNAME, count );
		Emit( fd, line );

		for( unsigned i = 0; i < count; i++ )
		{
			unsigned idx = ( pos - count + i ) % RING_SIZE;
			snprintf( line, sizeof( line ), "  %s\n", g_ring[idx] );
			Emit( fd, line );
		}
	}

	void DumpStackScan( int fd, uintptr_t sp )
	{
		Dl_info self;
		memset( &self, 0, sizeof( self ) );
		if( !sp || !dladdr( (void *)&g_ringPos, &self ) || !self.dli_fbase )
			return;

		char line[160];
		snprintf( line, sizeof( line ), "Pilha (varredura) - enderecos da lib %s:\n", HF_LIBNAME );
		Emit( fd, line );

		int printed = 0;
		const int words = 1024 / (int)sizeof( uintptr_t );
		uintptr_t *p = (uintptr_t *)( sp & ~(uintptr_t)( sizeof( uintptr_t ) - 1 ) );

		for( int i = 0; i < words && printed < kMaxScan; i++ )
		{
			uintptr_t v = p[i];
#if defined( __arm__ )
			if( !( v & 1 ) ) continue; // retornos em Thumb tem o bit 0 ligado
#endif
			uintptr_t a = Clean( v );
			Dl_info info;
			memset( &info, 0, sizeof( info ) );
			if( !a || !dladdr( (void *)a, &info ) || info.dli_fbase != self.dli_fbase )
				continue;

			snprintf( line, sizeof( line ), "  sp+%03d", (int)( i * sizeof( uintptr_t ) ) );
			Describe( fd, line, v );
			printed++;
		}
	}

	void Handler( int sig, siginfo_t *si, void *ucv )
	{
		alarm( 4 ); // se algo travar aqui dentro, o processo ainda morre

		int fd = OpenFile( "reprocessing_crash.log" );
		ucontext_t *uc = (ucontext_t *)ucv;
		uintptr_t pc = 0, lr = 0, sp = 0, fp = 0;

#if defined( __aarch64__ )
		if( uc ) { pc = uc->uc_mcontext.pc; lr = uc->uc_mcontext.regs[30]; sp = uc->uc_mcontext.sp; fp = uc->uc_mcontext.regs[29]; }
#elif defined( __arm__ )
		if( uc )
		{
			pc = uc->uc_mcontext.arm_pc; lr = uc->uc_mcontext.arm_lr; sp = uc->uc_mcontext.arm_sp;
			fp = ( uc->uc_mcontext.arm_cpsr & 0x20 ) ? uc->uc_mcontext.arm_r7 : uc->uc_mcontext.arm_fp; // Thumb usa r7
		}
#elif defined( __x86_64__ )
		if( uc ) { pc = uc->uc_mcontext.gregs[REG_RIP]; sp = uc->uc_mcontext.gregs[REG_RSP]; fp = uc->uc_mcontext.gregs[REG_RBP]; }
#elif defined( __i386__ )
		if( uc ) { pc = uc->uc_mcontext.gregs[REG_EIP]; sp = uc->uc_mcontext.gregs[REG_ESP]; fp = uc->uc_mcontext.gregs[REG_EBP]; }
#endif

		char line[320];

		if( !getenv( "RP_CRASH_LOGGED" ) )
		{
			setenv( "RP_CRASH_LOGGED", "1", 1 );

			snprintf( line, sizeof( line ), "\n==== CRASH: sinal %d (%s) code=%d addr=%p  [%s] ====\n",
				sig, strsignal( sig ), si ? si->si_code : 0, si ? si->si_addr : 0, si ? CodeText( sig, si->si_code ) : "" );
			Emit( fd, line );

			snprintf( line, sizeof( line ), "Quando: %lds depois de a lib carregar | pid %d | arquitetura %s | build %s %s\n",
				NowSec() - g_startSec, (int)getpid(),
#if defined( __aarch64__ )
				"arm64-v8a",
#elif defined( __arm__ )
				"armeabi-v7a (32 bits)",
#elif defined( __x86_64__ )
				"x86_64",
#else
				"x86",
#endif
				__DATE__, __TIME__ );
			Emit( fd, line );

			snprintf( line, sizeof( line ), "Registradores: pc=%p lr=%p sp=%p fp=%p\n", (void *)pc, (void *)lr, (void *)sp, (void *)fp );
			Emit( fd, line );

			Emit( fd, "Falha em:\n" );
			Describe( fd, "PC", pc );
			Emit( fd, "Chamador (LR):\n" );
			Describe( fd, "LR", lr );

			// cadeia por frame pointer: [fp] = fp anterior, [fp + 1 palavra] = endereco de retorno
			Emit( fd, "Cadeia (frame pointer):\n" );
			uintptr_t cur = fp;
			for( int i = 0; i < kMaxChain; i++ )
			{
				if( !cur || ( cur & ( sizeof( uintptr_t ) - 1 ) ) || cur < sp || cur > sp + 256 * 1024 )
					break;
				uintptr_t next = ( (uintptr_t *)cur )[0];
				uintptr_t ret = ( (uintptr_t *)cur )[1];
				if( !ret )
					break;
				char label[16];
				snprintf( label, sizeof( label ), "#%02d", i );
				Describe( fd, label, ret );
				if( next <= cur )
					break;
				cur = next;
			}
		}

		// parte propria de cada lib (client e server escrevem a sua)
		if( !g_dumped )
		{
			g_dumped = 1;
			DumpStackScan( fd, sp );
			DumpBreadcrumbs( fd );
		}

		Emit( fd, "====\n" );
		if( fd >= 0 )
			close( fd );

		// devolve ao handler original e deixa o sinal acontecer de novo
		if( sig > 0 && sig < 32 )
			sigaction( sig, &g_old[sig], NULL );
	}

	__attribute__(( constructor )) void InstallCrashHandler()
	{
		g_startSec = NowSec();

		stack_t ss;
		ss.ss_sp = g_altstack;
		ss.ss_size = sizeof( g_altstack );
		ss.ss_flags = 0;
		sigaltstack( &ss, NULL );

		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );
		sa.sa_sigaction = Handler;
		sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
		sigemptyset( &sa.sa_mask );
		for( int i = 0; i < kNumSignals; i++ )
			sigaction( kSignals[i], &sa, &g_old[kSignals[i]] );

		// evita que o log cresca sem parar
		FindDir();
		if( g_dir[0] )
		{
			char path[300];
			struct stat st;
			snprintf( path, sizeof( path ), "%sreprocessing_log.txt", g_dir );
			if( stat( path, &st ) == 0 && st.st_size > 512 * 1024 )
				truncate( path, 0 );
		}

		HF_Log( "==== lib %s carregada | pid %d | arquitetura %s | build %s %s ====", HF_LIBNAME, (int)getpid(),
#if defined( __aarch64__ )
			"arm64-v8a",
#elif defined( __arm__ )
			"armeabi-v7a (32 bits)",
#elif defined( __x86_64__ )
			"x86_64",
#else
			"x86",
#endif
			__DATE__, __TIME__ );
	}
}

void HF_Breadcrumb( const char *fmt, ... )
{
	unsigned idx = ( g_ringPos++ ) % RING_SIZE;
	char *dst = g_ring[idx];
	int n = snprintf( dst, RING_LEN, "[+%lds] ", NowSec() - g_startSec );
	if( n < 0 || n >= RING_LEN )
		return;

	va_list ap;
	va_start( ap, fmt );
	vsnprintf( dst + n, RING_LEN - n, fmt, ap );
	va_end( ap );
}

void HF_Log( const char *fmt, ... )
{
	char msg[400];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( msg, sizeof( msg ), fmt, ap );
	va_end( ap );

	HF_Breadcrumb( "%s", msg );

	char line[460];
	snprintf( line, sizeof( line ), "[%s +%lds] %s\n", HF_LIBNAME, NowSec() - g_startSec, msg );

	int fd = OpenFile( "reprocessing_log.txt" );
	if( fd >= 0 )
	{
		write( fd, line, strlen( line ) );
		close( fd );
	}
	LogcatWrite( 4 /*ANDROID_LOG_INFO*/, "RP", line );
}

int HF_FlagExists( const char *name )
{
	if( !g_dir[0] )
		FindDir();
	if( !g_dir[0] )
		return 0;

	char path[300];
	snprintf( path, sizeof( path ), "%s%s", g_dir, name );
	return access( path, F_OK ) == 0;
}

void HF_FlagSet( const char *name )
{
	int fd = OpenFile( name );
	if( fd >= 0 )
	{
		write( fd, "1\n", 2 );
		close( fd );
	}
}

#else // sem suporte (ex.: Windows): funcoes vazias

void HF_Breadcrumb( const char *fmt, ... ) { (void)fmt; }
void HF_Log( const char *fmt, ... ) { (void)fmt; }
int HF_FlagExists( const char *name ) { (void)name; return 0; }
void HF_FlagSet( const char *name ) { (void)name; }

#endif
