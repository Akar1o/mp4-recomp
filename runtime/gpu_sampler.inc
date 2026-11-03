/* SWR_SAMPLE_GPU=start_s:seconds (Linux): a sampling profile of the GPU thread. A timer thread interrupts it ~1000
   times a second and records the program counter; the histogram goes to gpu_samples.txt as offsets from the
   executable's load address (resolve with nm). For finding where the GPU thread's time goes without perf */
#if defined(__linux__) && defined(__x86_64__)
#include <signal.h>
#include <pthread.h>
#include <ucontext.h>
typedef struct { const char* dli_fname; void* dli_fbase; const char* dli_sname; void* dli_saddr; } Dl_info;   /* <dlfcn.h> hides it without _GNU_SOURCE */
extern int dladdr(const void*, Dl_info*);
#define GS_MAX (1u<<20)
#define GS_D 8
extern int backtrace(void**, int);
static uintptr_t* g_gs_pc; static uintptr_t (*g_gs_bt)[GS_D]; static volatile u32 g_gs_n; static pthread_t g_gs_target;
static void gs_handler(int sig, siginfo_t* si, void* uc){ (void)sig; (void)si; u32 i=__atomic_fetch_add(&g_gs_n,1,__ATOMIC_RELAXED);
  if(i<GS_MAX){ g_gs_pc[i]=(uintptr_t)((ucontext_t*)uc)->uc_mcontext.gregs[16] /* REG_RIP */; void* b[GS_D+2]; int n=backtrace(b,GS_D+2);
    for(int k=0;k<GS_D;k++) g_gs_bt[i][k]= k+2<n? (uintptr_t)b[k+2] : 0; } }
static int gs_cmp(const void* a, const void* b){ uintptr_t x=*(const uintptr_t*)a, y=*(const uintptr_t*)b; return x<y? -1 : x>y; }
static const char* g_gs_env="SWR_SAMPLE_GPU";
static void* gs_thread(void* arg){ (void)arg; const char* e=getenv(g_gs_env); double st=atof(e), du=10; const char* c=strchr(e,':'); if(c) du=atof(c+1);
  plat_sleep_ns((i64)(st*1e9)); LOG("sampler: profiling the GPU thread for %.0f s\n",du);
  u64 end=plat_ns()+(u64)(du*1e9); while(plat_ns()<end){ pthread_kill(g_gs_target,SIGPROF); plat_sleep_ns(1000000); }
  u32 n=g_gs_n<GS_MAX? g_gs_n : GS_MAX; qsort(g_gs_pc,n,sizeof *g_gs_pc,gs_cmp);
  Dl_info di; uintptr_t base=0; if(dladdr((void*)gs_thread,&di)) base=(uintptr_t)di.dli_fbase;
  char p[1200]; extern char g_exedir[]; snprintf(p,sizeof p,"%sgpu_samples.txt",g_exedir); FILE* f=fopen(p,"w");
  if(f){ fprintf(f,"%u samples, base %lx\n",n,(unsigned long)base); for(u32 i=0;i<n;){ u32 j=i; while(j<n && g_gs_pc[j]==g_gs_pc[i]) j++;
      uintptr_t pc=g_gs_pc[i]; Dl_info d2; const char* lib= dladdr((void*)pc,&d2) && d2.dli_fname? d2.dli_fname : "?"; uintptr_t lb= d2.dli_fbase? (uintptr_t)d2.dli_fbase : 0;
      fprintf(f,"%u %lx %s %s\n",j-i,(unsigned long)(pc-lb),lib,d2.dli_sname? d2.dli_sname : "-"); i=j; } fclose(f); }
  snprintf(p,sizeof p,"%sgpu_stacks.txt",g_exedir); f=fopen(p,"w");
  /* each frame as lib:symbol+offset (exported symbols; our own binary as an offset for nm) */
  if(f){ u32 m=g_gs_n<GS_MAX? g_gs_n : GS_MAX; for(u32 i=0;i<m;i++){ for(int k=-1;k<GS_D;k++){ uintptr_t a= k<0? g_gs_pc[0]*0+((uintptr_t)0) : g_gs_bt[i][k]; if(k<0) continue; if(!a) break;
        Dl_info d; if(dladdr((void*)a,&d) && d.dli_fname){ const char* ln=strrchr(d.dli_fname,'/'); ln= ln? ln+1 : d.dli_fname;
          if((uintptr_t)d.dli_fbase==base) fprintf(f,"@%lx ",(unsigned long)(a-base)); else if(d.dli_sname) fprintf(f,"%s:%s+%lx ",ln,d.dli_sname,(unsigned long)(a-(uintptr_t)d.dli_saddr)); else fprintf(f,"%s:+%lx ",ln,(unsigned long)(a-(uintptr_t)d.dli_fbase)); }
        else fprintf(f,"?%lx ",(unsigned long)a); } fprintf(f,"\n"); } fclose(f); }
  LOG("sampler: %u samples written to gpu_samples.txt\n",n); return NULL; }
static void gs_start_env(const char* env){ if(!getenv(env)) return; g_gs_env=env; g_gs_pc=calloc(GS_MAX,sizeof *g_gs_pc); g_gs_bt=calloc(GS_MAX,sizeof *g_gs_bt); { void* w[2]; backtrace(w,2); }   /* loads libgcc before the handler needs it */ g_gs_target=pthread_self();
  struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_sigaction=gs_handler; sa.sa_flags=SA_SIGINFO|SA_RESTART; sigemptyset(&sa.sa_mask); sigaction(SIGPROF,&sa,NULL);
  pthread_t th; pthread_create(&th,NULL,gs_thread,NULL); pthread_detach(th); }
static void gs_start(void){ gs_start_env("SWR_SAMPLE_GPU"); }
void gpu_sampler_glq(void){ gs_start_env("SWR_SAMPLE_GLQ"); }   /* glq.c: the GL thread */
/* SWR_STALL_STACK=1 (Linux, diagnostics): when one GL call on the GL thread has run 15 ms, its stack is logged once */
static volatile u64 g_st_t0; static volatile u32 g_st_id; static pthread_t g_st_th; static int g_st_on=-1; static void* g_st_bt[28]; static volatile int g_st_n;
static void st_handler(int sig, siginfo_t* si, void* uc){ (void)sig; (void)si; (void)uc; g_st_n=backtrace(g_st_bt,28); }
static void* st_watch(void* arg){ (void)arg; u32 last=0; int nlog=0;
  for(;;){ plat_sleep_ns(2000000); u64 t=g_st_t0; u32 id=g_st_id; if(!t || id==last || plat_ns()-t<15000000ull || nlog>=80) continue; last=id; nlog++;
    g_st_n=0; pthread_kill(g_st_th,SIGUSR2); for(int i=0;i<100 && !g_st_n;i++) plat_sleep_ns(100000);
    char buf[2400]; int o=0; for(int k=2;k<g_st_n && o<2200;k++){ Dl_info d; void* pc=g_st_bt[k]; if(dladdr(pc,&d) && d.dli_fname){ const char* ln=strrchr(d.dli_fname,'/'); ln= ln? ln+1 : d.dli_fname;
        o+=snprintf(buf+o,sizeof buf-o," %s+%lx",ln,(unsigned long)((char*)pc-(char*)d.dli_fbase)); } }
    buf[o]=0; LOG("stall stack (%.1f ms in):%s\n",(plat_ns()-t)/1e6,buf); }
  return NULL; }
void gs_stall_mark(int begin){
  if(g_st_on<0){ g_st_on=getenv("SWR_STALL_STACK")!=NULL; if(g_st_on){ g_st_th=pthread_self(); struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_sigaction=st_handler; sa.sa_flags=SA_SIGINFO|SA_RESTART;
      sigemptyset(&sa.sa_mask); sigaction(SIGUSR2,&sa,NULL); void* w[2]; backtrace(w,2); pthread_t th; pthread_create(&th,NULL,st_watch,NULL); pthread_detach(th); } }
  if(!g_st_on) return; if(begin){ g_st_id++; g_st_t0=plat_ns(); } else g_st_t0=0; }

#else
static void gs_start(void){}
void gpu_sampler_glq(void){}
void gs_stall_mark(int begin){ (void)begin; }
#endif
