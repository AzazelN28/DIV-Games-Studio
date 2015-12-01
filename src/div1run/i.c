// *** OJO *** Que el usuario pueda determinar de alguna forma imem_max
//             (o bien el n�mero de procesos m�ximo)

//�����������������������������������������������������������������������������
// Librerias utilizadas
//�����������������������������������������������������������������������������

#define DEFINIR_AQUI

#include "inter.h"
#include "divsound.h"
#include "cdrom.h"
#include "net.h"
#ifndef __APPLE__
#include <malloc.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// FILE PROTOTYPES
void interprete (void);
void crea_cuad(void);
void system_font(void);
void exec_process(void);
void finalizacion (void);
void elimina_proceso(int id);




// external prototypes
void InitHandler(int);
int DetectBlaster(int*,int*,int*,int*,int*);
int DetectGUS(int*,int*,int*,int*,int*);

extern int get_reloj();

void deb(void);

void readmouse(void);
void mouse_window(void);

int dll_loaded=0;

int trace_program=0;

int old_dump_type;
int old_restore_type;

int last_key_check,key_check,last_joy_check,joy_check; // Llamar al screensaver
int last_mou_check,mou_check;

//�����������������������������������������������������������������������������
// Critical error handler
//�����������������������������������������������������������������������������

unsigned c_e_0,c_e_1;
unsigned far * c_e_2;

int __far critical_error(unsigned deverr,unsigned errcode,unsigned far*devhdr)
  { c_e_0=deverr; c_e_1=errcode; c_e_2=devhdr; return(_HARDERR_IGNORE); }

//�����������������������������������������������������������������������������
void CMP_export(char *name,void *dir,int nparms)
{
#ifdef DIVDLL
static int nExt=0;
  nparms=nparms;
  name=name;
  ExternDirs[nExt++]=dir;
#endif
}

void CNT_export(char *name,void *dir,int nparms)
{
  nparms=nparms;
  dir=dir;
  name=name;
}

//�����������������������������������������������������������������������������

//�����������������������������������������������������������������������������
// Programa principal
//�����������������������������������������������������������������������������
void mainloop(void) {
    frame_start();
    #ifdef DEBUG
    if (kbdFLAGS[_F12] || trace_program) { trace_program=0; call_to_debug=1; }
    #endif
    old_dump_type=dump_type;
    old_restore_type=restore_type;
    do {
      #ifdef DEBUG
      if (call_to_debug) { call_to_debug=0; debug(); }
      #endif
      exec_process();
    } while (ide);
    frame_end();
}

int main(int argc,char * argv[]) {

  FILE * f;
  SDL_putenv("SDL_VIDEO_WINDOW_POS=center"); 
  atexit(SDL_Quit);
	SDL_Init(SDL_INIT_EVERYTHING);
  #ifndef DEBUG
  #ifndef __EMSCRIPTEN__
  if (argc<2) {
    printf("DIV32RUN Run time library - version 1.03b - Freeware by Hammer Technologies\n");
    printf("Error: Needs a DIV32RUN executable to load.");
    exit(0);
  }
  #endif
  #else
  vga_an=argc; // Para quitar un warning
  #endif

#ifdef DOS
  _harderr(critical_error);
#endif

  vga_an=320; vga_al=200;
  if ((mem=(int*)malloc(4*imem_max))!=NULL){
    memset(mem,0,4*imem_max);

#ifdef __EMSCRIPTEN__
f=fopen(HTML_EXE,"rb");
printf("FILE: %s %x\n",HTML_EXE,f);

#else

    if ((f=fopen(argv[1],"rb"))==NULL) {
      #ifndef DEBUG
      printf("Error: FILE NOT FOUND.");
      #endif
      exit(0); 
    }
#endif
	
    fseek(f,8819,SEEK_SET);
    fread(mem,4,imem_max,f); fclose(f);

    if ((mem[0]&128)==128) { trace_program=1; mem[0]-=128; }

    if (mem[0]!=0 && mem[0]!=1) {
      #ifndef DEBUG
      printf("Error: %d %d Needs a DIV32RUN executable to load.",mem[0],mem[1]);
      #endif
      exit(0);
    }

    kbdInit();
    if (mem[0]==0) {
		InitSound(); 
	}else {
#ifdef DOS
      InitHandler(0);
#endif
    }
#ifdef DOS
    Init_CD();
#endif
    interprete();

  } else exer(1);
}

//�����������������������������������������������������������������������������
// Inicializaci�n
//�����������������������������������������������������������������������������

int _mouse_x,_mouse_y;

void MAINSRV_Packet(WORD Usuario,WORD Comando,BYTE *Buffer,WORD Len);
void MAINNOD_Packet(WORD Usuario,WORD Comando,BYTE *Buffer,WORD Len);

extern int find_status;

void inicializacion (void) {
  FILE * f=NULL;
  int n;

  mouse=(struct _mouse*)&mem[long_header];
  scroll=(struct _scroll*)&mem[long_header+12];
  m7=(struct _m7*)&mem[long_header+12+10*10];
  joy=(struct _joy*)&mem[long_header+12+10*10+10*7];
  setup=(struct _setup*)&mem[long_header+12+10*10+10*7+8];

  if (mem[0]!=1) f=fopen("sound.cfg","rb");

  if (f!=NULL) {
    fread(setup,4,8,f); fclose(f);
  } else {
#ifdef DOS
    DetectBlaster(&setup->card,&setup->port,&setup->irq,&setup->dma,&setup->dma2);
    DetectGUS(&setup->card,&setup->port,&setup->irq,&setup->dma,&setup->dma2);
#endif
    setup->master=15; setup->sound_fx=15; setup->cd_audio=15;
  }

  iloc=mem[2];          // Inicio de la imagen de las variables locales
  iloc_len=mem[6]+mem[5]; // Longitud de las locales (p�blicas y privadas)
  iloc_pub_len=mem[6];	// Longitud de las variables locales p�blicas
  inicio_privadas=iloc_pub_len;
  itxt=mem[7];          // Inicio de la zona de textos
  imem=itxt+mem[8];     // Final de c�digo, locales y textos (longitud del cmp)

  if (iloc_len&1) iloc_len++; if (!(imem&1)) imem++;

  if((copia=(byte*)malloc(vga_an*vga_al))==NULL) exer(1);
  memset(copia,0,vga_an*vga_al);

  if((copia2=(byte*)malloc(vga_an*vga_al))==NULL) exer(1);
  memset(copia2,0,vga_an*vga_al);

  #ifdef DEBUG
  if((copia_debug=(char*)malloc(vga_an*vga_al))==NULL) exer(1);
  #endif

  if((ghost=(byte*)malloc(65536))==NULL) exer(1);

  crea_cuad();

//  if((texto=(struct t_texto *)malloc(sizeof(struct t_texto)*max_textos))==NULL) exer(1);

  // Crea los dos primeros procesos, init y main

  procesos=1; id_init=imem; imem+=iloc_len; id_start=id_end=imem;

  mem[iloc+_x1]=-1;

  memcpy(&mem[id_init],&mem[iloc],iloc_pub_len<<2); // *** Init
  mem[id_init+_Id]=id_init;
  mem[id_init+_IdScan]=id_init;
  mem[id_init+_Status]=2;

  memcpy(&mem[id_start],&mem[iloc],iloc_pub_len<<2); // *** Main
  mem[id_start+_Id]=id_start;
  mem[id_start+_IdScan]=id_start;
  mem[id_start+_Status]=2;
  mem[id_start+_IP]=mem[1];

  mem[id_init+_Son]=id_start;
  mem[id_start+_Father]=id_init;

  for (n=0;n<max_region;n++) {
    region[n].x0=0; region[n].y0=0;
    region[n].x1=vga_an; region[n].y1=vga_al;
  }

  memset(g,0,sizeof(g));

  if((g[0].grf=(int**)malloc(sizeof(int32_t*)*2000))==NULL) exer(1);
  memset(g[0].grf,0,sizeof(int32_t*)*2000); next_map_code=1000;

  memset(fonts,0,sizeof(fonts));
  memset(texto,0,sizeof(texto));

  system_font();

#ifdef DOS
  _bios_timeofday(_TIME_GETCLOCK,(long*)&ip); init_rnd(ip);
#endif
  //_setvideomode(_MRES256COLOR);

  svmode();
  mouse_on();
  //cbd.mouse_action=0;
  mouse_window();
  _mouse_x=mouse->x; _mouse_y=mouse->y;

  memset(&paleta[0],0,768); memset(&dac[0],0,768);
  dacout_r=0; dacout_g=0; dacout_b=0; dacout_speed=8;
  now_dacout_r=64; now_dacout_g=64; now_dacout_b=64; paleta_cargada=0;
  set_dac(); // tabla_ghost();

  mouse->z=-512;
  mouse->x=vga_an/2;
  mouse->y=vga_al/2;
  mouse->size=100;

  for (n=0;n<10;n++) { // En un principio no hay sistema de scroll, ni de m7
    (scroll+n)->z=512;
    (scroll+n)->ratio=200;
    (scroll+n)->region1=-1;
    (scroll+n)->region2=-1;

    (m7+n)->z=256;
    (m7+n)->height=32;
    (m7+n)->distance=64;
    (m7+n)->focus=256;
  }

  reloj=0; ultimo_reloj=0; freloj=ireloj=5.5; max_saltos=0;

  joy_timeout=0;

  #ifdef DEBUG
  debugger_step=0; call_to_debug=0; process_stoped=0;
  #endif

  saltar_volcado=0; volcados_saltados=0;

  init_sin_cos(); // Tablas de seno y coseno para el modo7

  #ifdef DEBUG
  init_debug(); new_palette=0; new_mode=0;
  #endif

  init_volcado();

#ifdef DIVDLL
  // DLL_3 Exportaci�n de funciones y variables (para utilizarlas en las DLL)

// Exportadas desde 'C'
// files
  DIV_export("div_fopen"  ,(void *)fopen  );
  DIV_export("div_fclose" ,(void *)fclose );
  DIV_export("div_malloc" ,(void *)malloc);
  DIV_export("div_free"   ,(void *)free);
  DIV_export("div_rand"   ,(void *)_random);
  DIV_export("div_text_out",(void *)text_out);
// variables publicas

  DIV_export("stack",(void *)&pila);
  DIV_export("sp",(void *)&sp);
  DIV_export("wide",(void *)&vga_an);
  DIV_export("height",(void *)&vga_al);
  DIV_export("buffer",(void *)&copia);
  DIV_export("background",(void *)&copia2);
  DIV_export("ss_time",(void *)&ss_time);
  DIV_export("ss_status",(void *)&ss_status);
  DIV_export("ss_exit",(void *)&ss_exit);

  DIV_export("process_size",(void *)&iloc_len);
  DIV_export("id_offset",(void *)&id);
  DIV_export("id_init_offset",(void *)&id_init);
  DIV_export("id_start_offset",(void *)&id_start);
  DIV_export("id_end_offset",(void *)&id_end);
  DIV_export("set_palette",(void *)&activar_paleta);
  DIV_export("ghost",(void *)&ghost);

  DIV_export("mem",(void *)mem);
  DIV_export("palette",(void *)paleta);
  DIV_export("active_palette",(void *)dac);
  DIV_export("key",(void *)kbdFLAGS);

  ss_time=3000; ss_time_counter=0;
  ss_status=1; activar_paleta=0;

/////////////////////////////////////////////////////////////

  COM_export=CNT_export;
  LookForAutoLoadDlls();
  COM_export=CMP_export;

#endif

#ifdef NETLIB
  inicializacion_red=0;

  SRV_Packet=MAINSRV_Packet;
  NOD_Packet=MAINNOD_Packet;
#endif

/////////////////////////////////////////////////////////////

  find_status=0;
}

//�����������������������������������������������������������������������������
//  Crea la tabla de cuadrados
//�����������������������������������������������������������������������������

void crea_cuad(void) {
  int a,b;

  if((cuad=(byte*)malloc(16384))==NULL) exer(1);

  a=0; do {
    b=0; do {
      * (int *) (cuad + a*4*64 + b*4) = (a>b) ? (a-b)*(a-b) : (b-a)*(b-a);
    } while (++b<64);
  } while (++a<64);
}

//�����������������������������������������������������������������������������
//      Carga el font del systema
//�����������������������������������������������������������������������������

#include "06x08.h"

byte * sys06x08=NULL;

void system_font(void) {
  int n,m;
  int * ptr;
  byte *si,*di,x;

  if ((sys06x08=(byte*)malloc(12288))==NULL) exer(1);

  si=(byte *)_06x08; di=sys06x08;

  for (n=0;n<1536;n++) { x=*si++;
    for (m=0;m<8;m++) { if (x&128) *di++=1; else *di++=0; x*=2; } }

  n=1356+sizeof(TABLAFNT)*256+12288;

  if ((fonts[0]=(byte*)malloc(n))==NULL) exer(1);

  memset(fonts[0],0,n);
  memcpy(fonts[0]+n-12288,sys06x08,12288);

  ptr=(int32_t*)(fonts[0]+1356);
  for (n=0;n<256;n++) {
    *ptr++=6; *ptr++=8; *ptr++=0; *ptr++=1356+sizeof(TABLAFNT)*256+n*48;
  } last_c1=1;

  f_i[0].ancho=6;
  f_i[0].espacio=3;
  f_i[0].espaciado=0;
  f_i[0].alto=8;
}

//�����������������������������������������������������������������������������
// Int�rprete del c�digo generado
//�����������������������������������������������������������������������������

int max,max_reloj;        // Para procesar seg�n _Priority y pintar seg�n _Z
extern int alt_x;
void interprete (void) {

  inicializacion();
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(mainloop, 0, 0);
#else
  while (procesos && !(kbdFLAGS[_ESC] && kbdFLAGS[_L_CTRL]) && !(alt_x)) {
	  mainloop();
  } finalizacion();
#endif
}

//�����������������������������������������������������������������������������
// Procesa el siguiente proceso
//�����������������������������������������������������������������������������

void exec_process(void) {
  ide=0; max=0x80000000;

  #ifdef DEBUG
  if (process_stoped) {
    id=ide=process_stoped;
    process_stoped=0;
    goto continue_process;
  }
  #endif

  id=id_old; do {
    if (mem[id+_Status]==2 && !mem[id+_Executed] &&
        mem[id+_Priority]>max) { ide=id; max=mem[id+_Priority]; }
    if (id==id_end) id=id_start; else id+=iloc_len;
  } while (id!=id_old);

  if (ide) if (mem[ide+_Frame]>=100) {
    mem[ide+_Frame]-=100;
    mem[ide+_Executed]=1;
  } else {
#ifdef NETLIB
    net_receive(); // Recibe los paquetes justo antes de ejecutar el proceso
#endif
    id=ide; ip=mem[id+_IP]; sp=64; pila[64]=0;

    #ifdef DEBUG
    continue_process:
    #endif

    max_reloj=get_reloj()+max_process_time;
//    printf("running bytecode: %d %d\n",ip+1,mem[ip+1]);
//printf("process entered at %d \n",ip);
  	do {
//		if((byte)mem[ip+1]>0)
//			printf("running bytecode: %d %d\n",ip,(byte)mem[ip]);
//	   if(ip>18819)
//		printf("fucking hell\n");
		switch ((byte)mem[ip++]) {
      case lnop: break;
      case lcar: pila[++sp]=mem[ip++]; break;
      case lasi:
//		printf("LASI: %d %d %d %d\n",ip-1,pila[sp],pila[sp-1],mem[pila[sp-1]]); 
		pila[sp-1]=mem[pila[sp-1]]=pila[sp]; 
//		printf("LASI: %d %d %d\n",pila[sp],pila[sp-1],mem[pila[sp-1]]); 
		sp--; 
		break;
      case lori: pila[sp-1]|=pila[sp]; sp--; break;
      case lxor: pila[sp-1]^=pila[sp]; sp--; break;
      case land: pila[sp-1]&=pila[sp]; sp--; break;
      case ligu: pila[sp-1]=pila[sp-1]==pila[sp]; sp--; break;
      case ldis: pila[sp-1]=pila[sp-1]!=pila[sp]; sp--; break;
      case lmay: pila[sp-1]=pila[sp-1]>pila[sp]; sp--; break;
      case lmen: pila[sp-1]=pila[sp-1]<pila[sp]; sp--; break;
      case lmei: pila[sp-1]=pila[sp-1]<=pila[sp]; sp--; break;
      case lmai: pila[sp-1]=pila[sp-1]>=pila[sp]; sp--; break;
      case ladd: pila[sp-1]+=pila[sp]; sp--; break;
      case lsub: pila[sp-1]-=pila[sp]; sp--; break;
      case lmul: pila[sp-1]*=pila[sp]; sp--; break;
      #ifdef DEBUG
      case ldiv:
        if (pila[sp]==0) {
          pila[--sp]=0; v_function=-2; e(e145);
          if (call_to_debug) { process_stoped=id; return; }
        } else {
          pila[sp-1]/=pila[sp]; sp--;
        } break;
      #else
      case ldiv: 
		if(pila[sp]!=0)
			pila[sp-1]/=pila[sp]; 
		
		sp--; 
		break;
      #endif
      #ifdef DEBUG
      case lmod:
        if (pila[sp]==0) {
          pila[--sp]=0; v_function=-2; e(e145);
          if (call_to_debug) { process_stoped=id; return; }
        } else {
          pila[sp-1]%=pila[sp]; sp--;
        } break;
      #else
      case lmod: 
		if(pila[sp]>0) 
			pila[sp-1]%=pila[sp]; 
		sp--; 
		break;
      #endif
      case lneg: pila[sp]=-pila[sp]; break;
      case lptr: pila[sp]=mem[pila[sp]]; break;
      case lnot: pila[sp]^=-1; break;
      case laid: pila[sp]+=id; break;
      case lcid: pila[++sp]=id; break;
      #ifdef DEBUG
      case lrng: if (pila[sp]<0 || pila[sp]>mem[ip]) {
          v_function=-2; e(e140);
          if (call_to_debug) { ip++; process_stoped=id; return; }
        } ip++; break;
      #else
      case lrng: ip++; break;
      #endif
      #ifdef DEBUG
      case ljmp: ip=mem[ip];
        if (reloj>max_reloj) {
          v_function=-2; e(e142); max_reloj=max_process_time+reloj;
          if (call_to_debug) { process_stoped=id; return; }
        } break;
      #else
      case ljmp: ip=mem[ip]; break;
      #endif
      #ifdef DEBUG
      case ljpf: if (pila[sp--]&1) ip++; else ip=mem[ip];
        if (reloj>max_reloj) {
          v_function=-2; e(e142); max_reloj=max_process_time+reloj;
          if (call_to_debug) { process_stoped=id; return; }
        } break;
      #else
      case ljpf: if (pila[sp--]&1) ip++; else ip=mem[ip]; break;
      #endif
      case lfun: function();
        #ifdef DEBUG
        if (call_to_debug) { process_stoped=id; return; }
        #endif
        break;
      case lcal:
//		printf("Spawning new process %d\n",ip);
        pila[++sp]=id2=id; pila[++sp]=ip+1; if (sp>long_pila) exer(3);
        procesos++; ip=mem[ip]; id=id_start;
        while (mem[id+_Status] && id<=id_end) id+=iloc_len;
        if (id>id_end) { if (id>imem_max-iloc_len) exer(2); id_end=id; }
        memcpy(&mem[id],&mem[iloc],iloc_pub_len<<2);
        mem[id+_Id]=id; mem[id+_Status]=2;
        if (mem[id+_BigBro]=mem[id2+_Son]) mem[mem[id+_BigBro]+_SmallBro]=id;
        mem[id2+_Son]=id; mem[id+_Father]=id2; break;
      case lret: elimina_proceso(id);
        bp=sp; sp-=mem[ip]+1; ip=pila[bp--]; bp=pila[bp]; pila[sp]=id;
        if (!ip) { id=bp; goto next_process1; }
        else { mem[id+_Executed]=1; id=bp; } break;
      case lasp: sp--; break;
      case lfrm: mem[id+_IdScan]=0; mem[id+_BlScan]=0;
        mem[id+_IP]=ip+1; bp=sp; sp-=mem[ip]+1; ip=pila[bp--]; bp=pila[bp];
        pila[sp]=id;
        if (!ip) { id=bp; goto next_process1; }
        else { mem[id+_Executed]=1; id=bp; } break;
      case lcbp: mem[id+_Param]=sp-mem[ip++]-1; break;
      case lcpa: mem[pila[sp--]]=pila[mem[id+_Param]++]; break;
      case ltyp: mem[id+_Bloque]=mem[ip++]; inicio_privadas=mem[6]; break;
      case lpri: memcpy(&mem[id+inicio_privadas],&mem[ip+1],(mem[ip]-ip-1)<<2);
        ip=mem[ip]; break;
      case lcse: if (pila[sp-1]==pila[sp]) ip++; else ip=mem[ip];
        sp--; break;
      case lcsr: if (pila[sp-2]>=pila[sp-1] && pila[sp-2]<=pila[sp]) ip++;
        else ip=mem[ip]; sp-=2; break;
      case lshr: pila[sp-1]>>=pila[sp]; sp--; break;
      case lshl: pila[sp-1]<<=pila[sp]; sp--; break;
      case lipt: pila[sp]=++mem[pila[sp]]; break;
      case lpti: pila[sp]=mem[pila[sp]]++; break;
      case ldpt: pila[sp]=--mem[pila[sp]]; break;
      case lptd: pila[sp]=mem[pila[sp]]--; break;
      case lada: pila[sp-1]=mem[pila[sp-1]]+=pila[sp]; sp--; break;
      case lsua: pila[sp-1]=mem[pila[sp-1]]-=pila[sp]; sp--; break;
      case lmua: pila[sp-1]=mem[pila[sp-1]]*=pila[sp]; sp--; break;
      #ifdef DEBUG
      case ldia:
        if (pila[sp]==0) {
          mem[pila[sp-1]]=0;
          pila[--sp]=0; v_function=-2; e(e145);
          if (call_to_debug) { process_stoped=id; return; }
        } else {
          pila[sp-1]=mem[pila[sp-1]]/=pila[sp]; sp--;
        } break;
      #else
      case ldia: 
		if(pila[sp]!=0)
			pila[sp-1]=mem[pila[sp-1]]/=pila[sp]; 
		sp--; 
		break;
      #endif
      #ifdef DEBUG
      case lmoa:
        if (pila[sp]==0) {
          mem[pila[sp-1]]=0;
          pila[--sp]=0; v_function=-2; e(e145);
          if (call_to_debug) { process_stoped=id; return; }
        } else {
          pila[sp-1]=mem[pila[sp-1]]%=pila[sp]; sp--;
        } break;
      #else
      case lmoa:
		if(pila[sp]!=0)
			pila[sp-1]=mem[pila[sp-1]]%=pila[sp]; 
		sp--; 
		break;
      #endif
      case lana: pila[sp-1]=mem[pila[sp-1]]&=pila[sp]; sp--; break;
      case lora: pila[sp-1]=mem[pila[sp-1]]|=pila[sp]; sp--; break;
      case lxoa: pila[sp-1]=mem[pila[sp-1]]^=pila[sp]; sp--; break;
      case lsra: pila[sp-1]=mem[pila[sp-1]]>>=pila[sp]; sp--; break;
      case lsla: pila[sp-1]=mem[pila[sp-1]]<<=pila[sp]; sp--; break;
      case lpar: inicio_privadas=mem[6]+mem[ip++]; break;
      case lrtf: elimina_proceso(id);
        bp=--sp; sp-=mem[ip]+1; ip=pila[bp--];
        if (!ip) { id=pila[bp]; pila[sp]=pila[bp+2]; goto next_process1; }
        else { mem[id+_Executed]=1; id=pila[bp]; pila[sp]=pila[bp+2]; } break;
      case lclo: procesos++; id2=id; id=id_start;
        while (mem[id+_Status] && id<=id_end) id+=iloc_len;
        if (id>id_end) { if (id>imem_max-iloc_len) exer(2); id_end=id; }
        memcpy(&mem[id],&mem[id2],iloc_len<<2);
        mem[id+_Id]=id; mem[id+_IP]=ip+1;
        if (mem[id+_BigBro]=mem[id2+_Son]) mem[mem[id+_BigBro]+_SmallBro]=id;
        mem[id+_SmallBro]=0; mem[id+_Son]=0;
        mem[id2+_Son]=id; mem[id+_Father]=id2;
        id=id2; ip=mem[ip]; break;
      case lfrf: mem[id+_IdScan]=0; mem[id+_BlScan]=0;
        mem[id+_Frame]+=pila[sp--];
        mem[id+_IP]=ip+1; bp=sp; sp-=mem[ip]+1; ip=pila[bp--];
        bp=pila[bp]; pila[sp]=id;
        if (!ip) { id=bp; goto next_process2; }
        else {
          if (mem[id+_Frame]>=100) { mem[id+_Frame]-=100; mem[id+_Executed]=1; }
          id=bp;
        } break;
      case limp:
#ifdef DIVDLL
        if ((pe[nDLL]=DIV_LoadDll((byte*)&mem[itxt+mem[ip++]]))==NULL)
          exer(4);
        else
          nDLL++;
#else
ip++;
printf("limp\n");
#endif
        break;
      case lext:
#ifdef DIVDLL
        call((unsigned int)ExternDirs[mem[ip++]]);
#else
ip++;
printf("lext\n");
#endif
        break;
      case lchk:
        if (pila[sp]<id_init || pila[sp]>id_end || pila[sp]!=mem[pila[sp]]) {
          v_function=-2; e(e141);
          #ifdef DEBUG
          if (call_to_debug) { process_stoped=id; return; }
          #endif
        }
        break;
      case ldbg:
        #ifdef DEBUG
        for (ibreakpoint=0;ibreakpoint<max_breakpoint;ibreakpoint++)
          if (breakpoint[ibreakpoint].line>-1 && breakpoint[ibreakpoint].offset==ip-1) break;
        if (ibreakpoint<max_breakpoint) { // Se lleg� a un breakpoint
          mem[--ip]=breakpoint[ibreakpoint].code;
          breakpoint[ibreakpoint].line=-1;
          call_to_debug=1; process_stoped=id; return;
        } deb();
        if (call_to_debug) return;
        #endif
        break;
      #include "cases.h"
    } 
    }while (1);

    next_process1: mem[ide+_Executed]=1;
    next_process2: ;
  }

  id=ide; if (post_process!=NULL) post_process();
}

//�����������������������������������������������������������������������������
// Procesa la siguiente instruccion del siguiente proceso
//�����������������������������������������������������������������������������

#ifdef DEBUG

void trace_process(void) {
  ide=0; max=0x80000000;

  if (process_stoped) {
    id=ide=process_stoped;
    process_stoped=0;
    goto continue_process;
  }

  id=id_old; do {
    if (mem[id+_Status]==2 && !mem[id+_Executed] &&
        mem[id+_Priority]>max) { ide=id; max=mem[id+_Priority]; }
    if (id==id_end) id=id_start; else id+=iloc_len;
  } while (id!=id_old);

  if (ide) if (mem[ide+_Frame]>=100) {
    mem[ide+_Frame]-=100;
    mem[ide+_Executed]=1;
  } else {
#ifdef NETLIB
    net_receive(); // Recibe los paquetes justo antes de ejecutar el proceso
#endif
    id=ide; ip=mem[id+_IP]; sp=64; pila[64]=0;
    continue_process:
    max_reloj=get_reloj()+max_process_time;
  	switch ((byte)mem[ip++]) {
      case lnop: break;
      case lcar: pila[++sp]=mem[ip++]; break;
      case lasi: pila[sp-1]=mem[pila[sp-1]]=pila[sp]; sp--; break;
      case lori: pila[sp-1]|=pila[sp]; sp--; break;
      case lxor: pila[sp-1]^=pila[sp]; sp--; break;
      case land: pila[sp-1]&=pila[sp]; sp--; break;
      case ligu: pila[sp-1]=pila[sp-1]==pila[sp]; sp--; break;
      case ldis: pila[sp-1]=pila[sp-1]!=pila[sp]; sp--; break;
      case lmay: pila[sp-1]=pila[sp-1]>pila[sp]; sp--; break;
      case lmen: pila[sp-1]=pila[sp-1]<pila[sp]; sp--; break;
      case lmei: pila[sp-1]=pila[sp-1]<=pila[sp]; sp--; break;
      case lmai: pila[sp-1]=pila[sp-1]>=pila[sp]; sp--; break;
      case ladd: pila[sp-1]+=pila[sp]; sp--; break;
      case lsub: pila[sp-1]-=pila[sp]; sp--; break;
      case lmul: pila[sp-1]*=pila[sp]; sp--; break;
      case ldiv:
        if (pila[sp]==0) {
          pila[--sp]=0; v_function=-2; e(e145);
          if (call_to_debug) { process_stoped=id; return; }
        } else {
          pila[sp-1]/=pila[sp]; sp--;
        } break;
      case lmod:
        if (pila[sp]==0) {
          pila[--sp]=0; v_function=-2; e(e145);
          if (call_to_debug) { process_stoped=id; return; }
        } else {
          pila[sp-1]%=pila[sp]; sp--;
        } break;
      case lneg: pila[sp]=-pila[sp]; break;
      case lptr: pila[sp]=mem[pila[sp]]; break;
      case lnot: pila[sp]^=-1; break;
      case laid: pila[sp]+=id; break;
      case lcid: pila[++sp]=id; break;
      case lrng: if (pila[sp]<0 || pila[sp]>mem[ip]) {
          v_function=-2; e(e140);
          if (call_to_debug) { ip++; process_stoped=id; return; }
        } ip++; break;
      case ljmp: ip=mem[ip];
        if (reloj>max_reloj) {
          v_function=-2; e(e142); max_reloj=max_process_time+reloj;
          if (call_to_debug) { process_stoped=id; return; }
        } break;
      case ljpf: if (pila[sp--]&1) ip++; else ip=mem[ip];
        if (reloj>max_reloj) {
          v_function=-2; e(e142); max_reloj=max_process_time+reloj;
          if (call_to_debug) { process_stoped=id; return; }
        } break;
      case lfun: function();
        if (call_to_debug) { process_stoped=id; return; }
        break;
      case lcal:
//		printf("Spawning new process %d\n",ip);
        pila[++sp]=id2=id; pila[++sp]=ip+1; if (sp>long_pila) exer(3);
        procesos++; ip=mem[ip]; id=id_start;
        while (mem[id+_Status] && id<=id_end) id+=iloc_len;
        if (id>id_end) { if (id>imem_max-iloc_len) exer(2); id_end=id; }
        memcpy(&mem[id],&mem[iloc],iloc_pub_len<<2);
        mem[id+_Id]=id; mem[id+_Status]=2;
        if (mem[id+_BigBro]=mem[id2+_Son]) mem[mem[id+_BigBro]+_SmallBro]=id;
        mem[id2+_Son]=id; mem[id+_Father]=id2; break;
      case lret: elimina_proceso(id);
        bp=sp; sp-=mem[ip]+1; ip=pila[bp--]; bp=pila[bp]; pila[sp]=id;
        if (!ip) { id=bp; goto next_process1; }
        else { mem[id+_Executed]=1; id=bp; } break;
      case lasp: sp--; break;
      case lfrm: mem[id+_IdScan]=0; mem[id+_BlScan]=0;
        mem[id+_IP]=ip+1; bp=sp; sp-=mem[ip]+1; ip=pila[bp--]; bp=pila[bp];
        pila[sp]=id;
        if (!ip) { id=bp; goto next_process1; }
        else { mem[id+_Executed]=1; id=bp; } break;
      case lcbp: mem[id+_Param]=sp-mem[ip++]-1; break;
      case lcpa: mem[pila[sp--]]=pila[mem[id+_Param]++]; break;
      case ltyp: mem[id+_Bloque]=mem[ip++]; inicio_privadas=mem[6]; break;
      case lpri: memcpy(&mem[id+inicio_privadas],&mem[ip+1],(mem[ip]-ip-1)<<2);
        ip=mem[ip]; break;
      case lcse: if (pila[sp-1]==pila[sp]) ip++; else ip=mem[ip];
        sp--; break;
      case lcsr: if (pila[sp-2]>=pila[sp-1] && pila[sp-2]<=pila[sp]) ip++;
        else ip=mem[ip]; sp-=2; break;
      case lshr: pila[sp-1]>>=pila[sp]; sp--; break;
      case lshl: pila[sp-1]<<=pila[sp]; sp--; break;
      case lipt: pila[sp]=++mem[pila[sp]]; break;
      case lpti: pila[sp]=mem[pila[sp]]++; break;
      case ldpt: pila[sp]=--mem[pila[sp]]; break;
      case lptd: pila[sp]=mem[pila[sp]]--; break;
      case lada: pila[sp-1]=mem[pila[sp-1]]+=pila[sp]; sp--; break;
      case lsua: pila[sp-1]=mem[pila[sp-1]]-=pila[sp]; sp--; break;
      case lmua: pila[sp-1]=mem[pila[sp-1]]*=pila[sp]; sp--; break;
      case ldia: pila[sp-1]=mem[pila[sp-1]]/=pila[sp]; sp--; break;
      case lmoa: pila[sp-1]=mem[pila[sp-1]]%=pila[sp]; sp--; break;
      case lana: pila[sp-1]=mem[pila[sp-1]]&=pila[sp]; sp--; break;
      case lora: pila[sp-1]=mem[pila[sp-1]]|=pila[sp]; sp--; break;
      case lxoa: pila[sp-1]=mem[pila[sp-1]]^=pila[sp]; sp--; break;
      case lsra: pila[sp-1]=mem[pila[sp-1]]>>=pila[sp]; sp--; break;
      case lsla: pila[sp-1]=mem[pila[sp-1]]<<=pila[sp]; sp--; break;
      case lpar: inicio_privadas=mem[6]+mem[ip++]; break;
      case lrtf: elimina_proceso(id);
        bp=--sp; sp-=mem[ip]+1; ip=pila[bp--];
        if (!ip) { id=pila[bp]; pila[sp]=pila[bp+2]; goto next_process1; }
        else { mem[id+_Executed]=1; id=pila[bp]; pila[sp]=pila[bp+2]; } break;
      case lclo: procesos++; id2=id; id=id_start;
        while (mem[id+_Status] && id<=id_end) id+=iloc_len;
        if (id>id_end) { if (id>imem_max-iloc_len) exer(2); id_end=id; }
        memcpy(&mem[id],&mem[id2],iloc_len<<2);
        mem[id+_Id]=id; mem[id+_IP]=ip+1;
        if (mem[id+_BigBro]=mem[id2+_Son]) mem[mem[id+_BigBro]+_SmallBro]=id;
        mem[id+_SmallBro]=0; mem[id+_Son]=0;
        mem[id2+_Son]=id; mem[id+_Father]=id2;
        id=id2; ip=mem[ip]; break;
      case lfrf: mem[id+_IdScan]=0; mem[id+_BlScan]=0;
        mem[id+_Frame]+=pila[sp--];
        mem[id+_IP]=ip+1; bp=sp; sp-=mem[ip]+1; ip=pila[bp--];
        bp=pila[bp]; pila[sp]=id;
        if (!ip) { id=bp; goto next_process2; }
        else {
          if (mem[id+_Frame]>=100) { mem[id+_Frame]-=100; mem[id+_Executed]=1; }
          id=bp;
        } break;
      case limp:
#ifdef DIVDLL
        if ((pe[nDLL]=DIV_LoadDll((byte*)&mem[itxt+mem[ip++]]))==NULL)
          exer(4);
        else
          nDLL++;
#else
ip++;
printf("limp\n");
#endif
        break;
      case lext:
#ifdef DIVDLL
        call((unsigned int)ExternDirs[mem[ip++]]);
#else
ip++;
printf("lext\n");
#endif
        break;
      #ifdef DEBUG
      case lchk:
        if (pila[sp]<id_init || pila[sp]>id_end || pila[sp]!=mem[pila[sp]]) {
          v_function=-2; e(e141);
          if (call_to_debug) { process_stoped=id; return; }
        } break;
      #else
      case lchk: break;
      #endif
      case ldbg:
        for (ibreakpoint=0;ibreakpoint<max_breakpoint;ibreakpoint++)
          if (breakpoint[ibreakpoint].line>-1 && breakpoint[ibreakpoint].offset==ip-1) break;
        if (ibreakpoint<max_breakpoint) { // Se lleg� a un breakpoint
          mem[--ip]=breakpoint[ibreakpoint].code;
          breakpoint[ibreakpoint].line=-1;
        } call_to_debug=1; process_stoped=id; return;

    } process_stoped=id; goto end_trace;

    next_process1: mem[ide+_Executed]=1;
    next_process2: ;
  }

  if (post_process!=NULL) post_process();
  end_trace: id=ide;
}

#endif

//�����������������������������������������������������������������������������
// Inicia un frame, prepara variables vuelca y espera timer
//�����������������������������������������������������������������������������

float ffps=18.2;

void frame_start(void) {

  int n;

  // Control del screen saver

  if (ss_status && ss_frame!=NULL) {
    if (reloj>ss_time_counter) {
      if (ss_init!=NULL) ss_init();
      ss_exit=0; do {
        key_check=0; for (n=0;n<128;n++) if (key(n)) key_check++;
        if (key_check!=last_key_check) ss_exit=1;
        readmouse();
        mou_check=mouse->x+mouse->y+mouse->left+mouse->right+mouse->middle;
        if (mou_check!=last_mou_check) ss_exit=1;
//        if (cbd.mouse_action) ss_exit=2;
        if (joy_status) {
          read_joy();
          joy_check=joy->button1+joy->button2+joy->left+joy->right+joy->up+joy->down;
          if (joy_check!=last_joy_check) ss_exit=3;
        }
        ss_frame();
        volcado_completo=1; volcado(copia);
      } while (!ss_exit);
      if (ss_end!=NULL) ss_end();
      memcpy(copia,copia2,vga_an*vga_al);
      volcado_parcial(0,0,vga_an,vga_al);
      ss_time_counter=get_reloj()+ss_time;
    }
  }

  // Elimina los procesos muertos

  for (ide=id_start; ide<=id_end; ide+=iloc_len)
    if (mem[ide+_Status]==1) elimina_proceso(ide);

  // Si se est� haciendo un fade lo contin�a

  if (now_dacout_r!=dacout_r || now_dacout_g!=dacout_g || now_dacout_b!=dacout_b) {
    set_paleta();
    set_dac();
    fading=1;
  } else {
    if (activar_paleta) {
      set_paleta();
      set_dac();
      activar_paleta--;
    } fading=0;
  }

  for (max=0;max<10;max++) timer(max)+=get_reloj()-ultimo_reloj;

  if (reloj>ultimo_reloj) {
    ffps=(ffps*49.0+100.0/(float)(reloj-ultimo_reloj))/50.0;
    fps=(int)ffps;
  }

  ultimo_reloj=get_reloj();

  //LoopSound();

  tecla();

  if (reloj>(freloj+ireloj/3)) { // Permite comerse hasta un tercio del sgte frame
    if (volcados_saltados<max_saltos) {
      volcados_saltados++;
      saltar_volcado=1;
      freloj+=ireloj;
    } else {
      freloj=(float)reloj+ireloj;
      volcados_saltados=0;
      saltar_volcado=0;
    }
  } else {
    do { } while (get_reloj()<(int)freloj); // Espera para no dar m�s de "n" fps
    volcados_saltados=0;
    saltar_volcado=0;
    freloj+=ireloj;
  }

  // Marca todos los procesos como no ejecutados

  for (ide=id_start; ide<=id_end; ide+=iloc_len) mem[ide+_Executed]=0;

  // Fija la prioridad m�xima, para ejecutar procesos seg�n _Priority

  id_old=id_start; // El siguiente a procesar

  // Posiciona las variables del rat�n

//  if (cbd.mouse_action) {
    readmouse();
/*
    _mouse_x=mouse->x=cbd.mouse_cx>>1;
    _mouse_y=mouse->y=cbd.mouse_dx;
    mouse->left=(cbd.mouse_bx&1);
    mouse->middle=(cbd.mouse_bx&4)/4;
    mouse->right=(cbd.mouse_bx&2)/2;
    cbd.mouse_action=0;
    ss_time_counter=reloj+ss_time;
  }
*/

  // Lee el joystick

  if (joy_status==1 && joy_timeout>=6) {
    joy->button1=0; joy->button2=0;
    joy->button3=0; joy->button4=0;
    joy->left=0; joy->right=0;
    joy->up=0; joy->down=0;
    joy_status=0;
  } else if (joy_status) {
    read_joy();
    joy_check=joy->button1+joy->button2+joy->left+joy->right+joy->up+joy->down;
    if (joy_check!=last_joy_check) {
      last_joy_check=joy_check;
      ss_time_counter=get_reloj()+ss_time;
    }
  }

  key_check=0; for (n=0;n<128;n++) if (key(n)) key_check++;
  if (key_check!=last_key_check) {
    last_key_check=key_check;
    ss_time_counter=get_reloj()+ss_time;
  }

  mou_check=mouse->x+mouse->y+mouse->left+mouse->right+mouse->middle;
  if (mou_check!=last_mou_check) {
    last_mou_check=mou_check;
    ss_time_counter=get_reloj()+ss_time;
  }

}

//�����������������������������������������������������������������������������
// Finaliza un frame e imprime los gr�ficos
//�����������������������������������������������������������������������������

void frame_end(void) {
  int mouse_pintado=0,textos_pintados=0;
  int mouse_x0,mouse_x1,mouse_y0,mouse_y1;
  int n,m7ide,scrollide,otheride;

  // DLL_0 Lee los puntos de ruptura (bien sea de autoload o de import)
#ifdef DIVDLL
  if (!dll_loaded) {
    dll_loaded=1;

    // Los importa

    set_video_mode        =DIV_import("set_video_mode"); //ok
    process_palette       =DIV_import("process_palette"); //ok
    process_active_palette=DIV_import("process_active_palette"); //ok

    process_sound         =DIV_import("process_sound"); //ok

    process_fpg           =DIV_import("process_fpg"); //ok
    process_map           =DIV_import("process_map"); //ok
    process_fnt           =DIV_import("process_fnt"); //ok

    background_to_buffer  =DIV_import("background_to_buffer"); //ok
    buffer_to_video       =DIV_import("buffer_to_video"); //ok

    post_process_scroll   =DIV_import("post_process_scroll"); //ok
    post_process_m7       =DIV_import("post_process_m7"); //ok
    post_process_buffer   =DIV_import("post_process_buffer"); //ok
    post_process          =DIV_import("post_process"); //ok

    putsprite             =DIV_import("put_sprite"); //ok

    ss_init               =DIV_import("ss_init"); //ok
    ss_frame              =DIV_import("ss_frame"); //ok
    ss_end                =DIV_import("ss_end"); //ok

    ss_time_counter=get_reloj()+ss_time;

    // DLL_1 Aqu� se llama a uno.

    #ifdef DEBUG
    if (process_palette!=NULL)
    {
      process_palette();
    }
    #endif
  }
#endif
  // Si el usuario modific� mouse.x o mouse.y, posiciona el rat�n debidamente
  if (_mouse_x!=mouse->x || _mouse_y!=mouse->y) set_mouse(mouse->x,mouse->y);

  if (!saltar_volcado) {

    // *** OJO *** Restaura las zonas de copia fuera del scroll y del modo 7

    if (restore_type==0 || restore_type==1) {
      if (!iscroll[0].on || iscroll[0].x || iscroll[0].y || iscroll[0].an!=vga_an || iscroll[0].al!=vga_al) {
        if (background_to_buffer!=NULL) background_to_buffer(); else {
          if (old_restore_type==0) restore(copia,copia2);
          else memcpy(copia,copia2,vga_an*vga_al);
        }
      }
    }

    // Pinta los sprites, por orden de _Z (a mayor z se pinta antes)

    for (ide=id_start; ide<=id_end; ide+=iloc_len) {
      mem[ide+_Executed]=0; // Sin ejecutar
      mem[ide+_x1]=-1; // Sin regi�n de volcado
    }

    for (n=0;n<10;n++) { im7[n].painted=0; iscroll[n].painted=0; }

    do { ide=0; m7ide=0; scrollide=0; otheride=0; max=0x80000000;

      for (id=id_start; id<=id_end; id+=iloc_len)
      	if ((mem[id+_Status]==2 || mem[id+_Status]==4) && mem[id+_Ctype]==0 &&
      	    !mem[id+_Executed] && mem[id+_Z]>max) { ide=id; max=mem[id+_Z]; }

      for (n=0;n<10;n++)
      	if (im7[n].on && (m7+n)->z>=max && !im7[n].painted) {
      	  m7ide=n+1; max=(m7+n)->z;
      	}

      for (n=0;n<10;n++)
      	if (iscroll[n].on && (scroll+n)->z>=max && !iscroll[n].painted) {
      	  scrollide=n+1; max=(scroll+n)->z;
      	}

      if (text_z>=max && !textos_pintados) { max=text_z; otheride=1; }

      if (mouse->z>=max && !mouse_pintado) { max=mouse->z; otheride=2; }

      if (otheride) {
      	if (otheride==1) {
      	  pinta_textos(); textos_pintados=1;
      	} else if (otheride==2) {
          readmouse();
//      	  if (cbd.mouse_action) { // Para evitar retardos (en lo posible)
//      	    _mouse_x=mouse->x=cbd.mouse_cx>>1;
//      	    _mouse_y=mouse->y=cbd.mouse_dx;
//      	    mouse->left=(cbd.mouse_bx&1);
//      	    mouse->middle=(cbd.mouse_bx&4)/4;
//      	    mouse->right=(cbd.mouse_bx&2)/2;
//      	    cbd.mouse_action=0;
//            ss_time_counter=reloj+ss_time;
//          }
          x1s=-1; v_function=-1; // No se producen errores
          put_sprite(mouse->file,mouse->graph,mouse->x,mouse->y,mouse->angle,mouse->size,mouse->flags,mouse->region,copia,vga_an,vga_al);
          mouse_x0=x0s;mouse_x1=x1s;mouse_y0=y0s;mouse_y1=y1s; mouse_pintado=1;
      	}
      } else if (scrollide) {
      	iscroll[snum=scrollide-1].painted=1;
      	if (iscroll[snum].on==1) scroll_simple();
      	else if (iscroll[snum].on==2) scroll_parallax();
      } else if (m7ide) {
      	pinta_m7(m7ide-1); im7[m7ide-1].painted=1;
      } else if (ide) {
      	pinta_sprite(); mem[ide+_Executed]=1;
      }

    } while (ide || m7ide || scrollide || otheride);

    if (post_process_buffer!=NULL)
    {
      post_process_buffer();
    }

    #ifdef DEBUG
    if (!debugger_step)
    #endif

    if (buffer_to_video!=NULL) buffer_to_video(); else {
      if (old_dump_type) {

        volcado_completo=1; volcado(copia);

      } else {

        volcado_completo=0;

        // A�ade los volcados de este frame a los restore del anterior

        for (n=id_start; n<=id_end; n+=iloc_len)
          if (mem[n+_x1]!=-1) volcado_parcial(mem[n+_x0],mem[n+_y0],mem[n+_x1]-mem[n+_x0]+1,mem[n+_y1]-mem[n+_y0]+1);
        for (n=0;n<10;n++) {
          if (im7[n].on) volcado_parcial(im7[n].x,im7[n].y,im7[n].an,im7[n].al);
          if (iscroll[n].on) volcado_parcial(iscroll[n].x,iscroll[n].y,iscroll[n].an,iscroll[n].al);
        }
        if (mouse_x1!=-1) volcado_parcial(mouse_x0,mouse_y0,mouse_x1-mouse_x0+1,mouse_y1-mouse_y0+1);
        for (n=0;n<max_textos;n++) if (texto[n].font && texto[n].an)
            volcado_parcial(texto[n].x0,texto[n].y0,texto[n].an,texto[n].al);

        // Realiza un volcado parcial

        volcado(copia);

      }

      if (dump_type==0 || restore_type==0) { // Fija los restore para el siguiente frame

        for (n=id_start; n<=id_end; n+=iloc_len)
          if (mem[n+_x1]!=-1) volcado_parcial(mem[n+_x0],mem[n+_y0],mem[n+_x1]-mem[n+_x0]+1,mem[n+_y1]-mem[n+_y0]+1);
        for (n=0;n<10;n++) {
          if (im7[n].on) volcado_parcial(im7[n].x,im7[n].y,im7[n].an,im7[n].al);
          if (iscroll[n].on) volcado_parcial(iscroll[n].x,iscroll[n].y,iscroll[n].an,iscroll[n].al);
        }
        if (mouse_x1!=-1) volcado_parcial(mouse_x0,mouse_y0,mouse_x1-mouse_x0+1,mouse_y1-mouse_y0+1);
        for (n=0;n<max_textos;n++) if (texto[n].font && texto[n].an)
            volcado_parcial(texto[n].x0,texto[n].y0,texto[n].an,texto[n].al);
      }

    }

  }

}

//�����������������������������������������������������������������������������
// Elimina un proceso
//�����������������������������������������������������������������������������

void elimina_proceso(int id) {
  int id2;

  mem[id+_Status]=0; procesos--;
  if (id2=mem[id+_Father])
    if (mem[id2+_Son]==id) mem[id2+_Son]=mem[id+_BigBro];
  if (id2=mem[id+_BigBro]) mem[id2+_SmallBro]=mem[id+_SmallBro];
  if (id2=mem[id+_SmallBro]) mem[id2+_BigBro]=mem[id+_BigBro];
  if (id2=mem[id+_Son]) {
    do {
      mem[id2+_Father]=id_init;
      if (mem[id2+_BigBro]==0) {
        mem[id2+_BigBro]=mem[id_init+_Son];
        mem[mem[id_init+_Son]+_SmallBro]=id2; id2=0;
      } else id2=mem[id2+_BigBro];
    } while (id2);
    mem[id_init+_Son]=mem[id+_Son];
  }
}

//�����������������������������������������������������������������������������
// Finalizaci�n
//�����������������������������������������������������������������������������

void finalizacion (void) {
#ifdef DIVDLL
  while (nDLL--) DIV_UnLoadDll(pe[nDLL]);
#endif
  dacout_r=64; dacout_g=64; dacout_b=64; dacout_speed=4;
  while (now_dacout_r!=dacout_r || now_dacout_g!=dacout_g || now_dacout_b!=dacout_b) {
    set_paleta(); set_dac();
  }
#ifdef NETLIB
  if (inicializacion_red) net_end();
#endif
  rvmode();
  EndSound();
  mouse_off();
  kbdReset();

//  printf("Ejecuci�n correcta:\n\n\tn� actual de procesos = %u\n\tn� m�ximo de procesos = %u",
//    procesos,(id_end-id_start)/iloc_len+1);

}

//�����������������������������������������������������������������������������
// Error de ejecuci�n
//�����������������������������������������������������������������������������

void exer(int e) {

  #ifdef DEBUG

  FILE *f;

  if (e) {
    if ((f=fopen("system\\exec.err","wb"))!=NULL) {
      fwrite(&e,4,1,f);
      fclose(f);
    }
  }

  #else

  if (e) {
    printf("Error: ");
    switch(e) {
      case 1: printf("Out of memory!"); break;
      case 2: printf("Too many process!"); break;
      case 3: printf("Stack overflow!"); break;
      case 4: printf("DLL not found!"); break;
      case 5: printf("System font file missed!"); break;
      case 6: printf("System graphic file missed!"); break;
      default: printf("Internal error!"); break;
    }
  }

  #endif

  //printf("*** Error de ejecuci�n:\n\n\tn� actual de procesos = %u\n\tn� m�ximo de procesos = %u",
  //procesos,(id_end-id_start)/iloc_len+1);

#ifdef NETLIB
  if (inicializacion_red) net_end();
#endif

  rvmode();
  EndSound();
  mouse_off();
  kbdReset();

  exit(0);
}

//����������������������������������������������������������������������������
//      Mensajes de error - Versi�n con debugger
//����������������������������������������������������������������������������

char * fname[]={
"signal","key","load_pal","load_fpg","start_scroll","stop_scroll","out_region",
"graphic_info","collision","get_id","get_distx","get_disty","get_angle",
"get_dist","fade","load_fnt","write","write_int","delete_text","move_text",
"unload_fpg","rand","define_region","xput","put","put_screen","map_xput",
"map_put","put_pixel","get_pixel","map_put_pixel","map_get_pixel","get_point",
"clear_screen","save","load","set_mode","load_pcm","unload_pcm","sound",
"stop_sound","change_sound","set_fps","start_fli","frame_fli","end_fli",
"reset_fli","system","refresh_scroll","fget_dist","fget_angle","play_cd",
"stop_cd","is_playing_cd","start_mode7","stop_mode7","advance","abs","fade_on",
"fade_off","rand_seed","sqrt","pow","map_block_copy","move_scroll",
"near_angle","let_me_alone","exit","roll_palette","get_real_point",
"get_joy_button","get_joy_position","convert_palette","load_map","reset_sound",
"unload_map","unload_fnt","set_volume"};

#ifndef DEBUG

void e(char * texto) {

  if (v_function==-1) return;

  if (v_function>=0)
       printf("DIV Execution error: %s (%s)",texto,fname[v_function]);
  else printf("DIV Execution error: %s",texto);

#ifdef NETLIB
  if (inicializacion_red) net_end();
#endif

  rvmode();
  EndSound();
  mouse_off();
//  if (end_extern!=NULL) end_extern();
  kbdReset();

  exit(0);
}

#endif
