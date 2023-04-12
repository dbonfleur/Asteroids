#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <math.h>
#include <mysql.h>
#include <string.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>

#define ANG 0.15707975
#define PX_NAVE 7
#define PX_ASTE 2
#define PX_TIRO 10
#define R_ASTE1 85
#define R_ASTE2 50
#define R_ASTE3 25
#define R_NAVE 31.25
#define R_TIRO 5
#define FPS 0.018

#define HOST "localhost"
#define USER "root"
#define PASS "Stalker543"
#define DB "asteroids"

//Variaveis do tipo allegro para a manipulação de imagens, fontes, eventos e janelas.
ALLEGRO_DISPLAY *janela=NULL;
ALLEGRO_FONT *fonte90=NULL;
ALLEGRO_FONT *fonte70=NULL;
ALLEGRO_FONT *fonte48=NULL;
ALLEGRO_FONT *fonte24=NULL;
ALLEGRO_BITMAP *nav=NULL;
ALLEGRO_BITMAP *vida=NULL;
ALLEGRO_BITMAP *prop=NULL;
ALLEGRO_BITMAP *tiro=NULL;
ALLEGRO_BITMAP *ast=NULL;
ALLEGRO_BITMAP *ast2=NULL;
ALLEGRO_BITMAP *ast3=NULL;
ALLEGRO_BITMAP *fnd_ini=NULL;
ALLEGRO_BITMAP *logo=NULL;
ALLEGRO_BITMAP *bilunave=NULL;
ALLEGRO_EVENT_QUEUE *fila_eventos=NULL;
MYSQL cnx;
MYSQL_RES *pesq;
MYSQL_ROW linhas;

int TELA_LARG, TELA_ALT, opc_menu, tec_inicial;
int opc_user, tec_inicial_user, cod_user, flag_records;
int count_criar_user, resultSql, qtd_usuarios, qtd_records;
char let[3], nome_user[15], comando[255], **usuariosExistentes;

struct Aux {
    int quant_aste;
    int ordem_aste;
    int score;
    int cont_game_over;
    int atual_stage;
    int cont_stage;
    int cont_direcional;
    int flag_direita;
    int flag_esquerda;
    int qtd_ast_destr;
}aux;

//Estrutura que realiza movimentação da nave.
struct Movimento {
    float pix1;//Variavel que indicara quantos pixeis se movimentara pela tela em direção as linhas a cada 170 millesegundos.
    float pix2;//Variavel que indicara quantos pixeis se movimentara pela tela em direção as colunas a cada 170 millesegundos.
    float sub1;//Variavel auxiliar para realizar a movimentação de inércia em direção as linhas.
    float sub2;//Variavel auxiliar para realizar a movimentação de inércia em direção as colunas.
    int acelera;//Indica quando inicia a movimentação a partir do zero, realizando a aceleração.
    int constante;//Indica que a movimentação se tornou constante.
    int desacelera;//Indica quando inicia a movimentação de incercia após parar de acelerar, realizando a aceleração.
    int flag_acelera;//Flag que fornece valores a variaveis quando a aceleração se inicia.
    int flag_desacelera;//Flag que fornece valores a variaveis quando a desaceleração se inicia.
    int cont_acelera;//Contador que fornece uma quantidade de tempo para a aceleração acontecer.
    int cont_desacelera;//Contador que fornece uma quantidad de tempo para a inércia acontecer.
}movi;

//Estrutura que fornece as informações da nave em tempo de clock.
struct Nave {
    float bit_pi;//Posição de pixel na janela em relação as linhas.
    float bit_pj;//Posição de pixel na janela em relação as colunas.
    int angulo;//Angulação da nave ao realizar uma nova direção para a nave.
    int angulo_des;//Angulação auxiliar que informará a que angulo a inércia deve acontecer, independente da angulação original da nave.
    int flag_tiro;//Flag que indica que o disparo esta sendo efetuado.
    int intv_tiro;//Contador que dará um intervalo entre um disparo e o próximo.
    int seq_tiro;//Variavel que informa em qual vetor de estrura de tiro está disponivel para efetuar o disparo.
    int vet_tiro[6];//Vetor que indicara que se estiver '0' o disparo não foi efetuado, caso '1' então o disparo está se movimentado na tela.
    int colisao;
    int destruido;
    int vidas;
    int cont_renascer;
    int aste_dest;
    int cont_pass_level;
}nave;

//Estrutura que fornece as informações dos asteroids em tempo de clock.
struct Aste {
    float ast_pi;//Posição do asteroid no campo em relação as linhas.
    float ast_pj;//Posição do asteroid no campo em relação as colunas.
    float pixi;//Constante que informara a velocidade em pixels em relação as linhas.
    float pixj;//Constante que informara a velocidade em pixels em relação as colunas.
    float angulo;//Angulação de rotação do asteroids, variavel se altera constantemente.
    int angulo_dir;//Angulação onde é realizado a rota de caminho pelo espaço.
    int flag_destruido;//Flag que informa quando um asteroid é destruido.
    int tipo;
    int livre;
};

//Criação de um tipo de struct Aste, que servirá para a alocação dinâmica.
typedef struct Aste ASTE;
ASTE **aste;

struct Records {
    char nome[16];
    char pontos[15];
    char stage[3];
    char qtd_asteroids[5];
};

typedef struct Records RECORDS;
RECORDS **records;

struct Tir0 {
    float tiro_pi;
    float tiro_pj;
    int tiro_cont;
    int angulo_tiro;
}tir0[6];

void error_msg(char *text){
    al_show_native_message_box(janela,"ERRO","Ocorreu o seguinte erro e o programa sera finalizado:",text,NULL,ALLEGRO_MESSAGEBOX_ERROR);
}

int ini_alle() {
    if (!al_init()){
        error_msg("Falha ao inicializar a Allegro");
        return 0;
    }
    if (!al_init_image_addon()) {
        error_msg("Falha ao inicializar o Addon");
        return 0;
    }
    if (!al_install_mouse()) {
        error_msg("Falha ao inicializar o mouse");
        return 0;
    }
    if (!al_install_keyboard()){
        error_msg("Falha ao inicializar o teclado");
        return 0;
    }
    al_init_font_addon();
    if (!al_init_ttf_addon()){
        error_msg("Falha ao inicializar add-on allegro_ttf");
        return 0;
    }
    fonte90=al_load_font("neoletters.ttf", 90, 0);
    if (!fonte90) {
        error_msg("Falha ao carregar fonte 90");
        return 0;
    }
    fonte70=al_load_font("neoletters.ttf", 70, 0);
    if (!fonte70) {
        error_msg("Falha ao carregar fonte 70");
        return 0;
    }
    fonte48=al_load_font("neoletters.ttf", 48, 0);
    if (!fonte48){
        error_msg("Falha ao carregar fonte 48");
        return 0;
    }
    fonte24=al_load_font("neoletters.ttf", 24, 0);
    if (!fonte24){
        error_msg("Falha ao carregar fonte 24");
        return 0;
    }
    fila_eventos=al_create_event_queue();
    if (!fila_eventos){
        error_msg("Falha ao criar fila de eventos");
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        return 0;
    }
    TELA_LARG = GetSystemMetrics(SM_CXSCREEN);
    TELA_ALT = GetSystemMetrics(SM_CYSCREEN);
    janela=al_create_display(TELA_LARG, TELA_ALT);
    al_set_display_flag(janela, ALLEGRO_FULLSCREEN_WINDOW, 1);
    if (!janela){
        error_msg("Falha ao criar janela");
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_event_queue(fila_eventos);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        return 0;
    }
    nav=al_load_bitmap("imagens/nave.png");
    if (!nav){
        error_msg("Falha ao caregar nave.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    vida=al_load_bitmap("imagens/vidas.png");
    if (!vida) {
        error_msg("Falha ao caregar nave.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    tiro=al_load_bitmap("imagens/tiro.png");
    if (!tiro){
        error_msg("Falha ao caregar tiro.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    logo=al_load_bitmap("imagens/asteroids_logo.png");
    if (!logo) {
        error_msg("Falha ao caregar tiro.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    fnd_ini=al_load_bitmap("imagens/fnd_ini.png");
    if (!fnd_ini) {
        error_msg("Falha ao caregar fnd_ini.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    ast=al_load_bitmap("imagens/aste1.png");
    if (!ast){
        error_msg("Falha ao caregar aste1.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    ast2=al_load_bitmap("imagens/aste2.png");
    if (!ast2){
        error_msg("Falha ao caregar aste2.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    ast3=al_load_bitmap("imagens/aste3.png");
    if (!ast3){
        error_msg("Falha ao caregar aste3.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    bilunave=al_load_bitmap("imagens/etbilu.png");
    if (!bilunave){
        error_msg("Falha ao caregar aste3.png");
        al_destroy_display(janela);
        al_destroy_font(fonte24);
        al_destroy_font(fonte48);
        al_destroy_font(fonte70);
        al_destroy_font(fonte90);
        al_destroy_event_queue(fila_eventos);
        return 0;
    }
    al_set_window_title(janela, "Asteroids");
    al_set_window_position(janela,0,0);
    al_register_event_source(fila_eventos, al_get_mouse_event_source());
    al_register_event_source(fila_eventos, al_get_keyboard_event_source());
    al_register_event_source(fila_eventos, al_get_display_event_source(janela));
    cod_user = -1;
    opc_user = 0;
    tec_inicial_user = 1;
    count_criar_user = 0;
    return 1;
}

void ini_parametros() {
    int i;
    srand(time(NULL));
    for(i=0;i<6;i++) {
        nave.vet_tiro[i]=0;
    }
    nave.bit_pi             =TELA_ALT/2;
    nave.bit_pj             =TELA_LARG/2;
    nave.angulo             =0;
    nave.colisao            =0;
    nave.seq_tiro           =0;
    nave.intv_tiro          =0;
    nave.destruido          =0;
    nave.vidas              =3;
    nave.aste_dest          =0;
    nave.cont_pass_level    =200;
    al_set_mouse_z(0);
    aux.quant_aste          =2;
    aux.ordem_aste          =aux.quant_aste;
    aux.score               =0;
    aux.cont_game_over      =80;
    aux.atual_stage         =0;
    aux.cont_stage          =100;
    aux.cont_direcional     =1;
    aux.qtd_ast_destr       =0;
    opc_menu                =0;
    tec_inicial             =1;
    flag_records            =0;
    aloca_aste();
}

void sqlConsultUsuarios() {
    resultSql = mysql_query(&cnx, "select * from tb_user");
    if(!resultSql) {
        pesq=mysql_store_result(&cnx);
        qtd_usuarios = mysql_num_rows(pesq);
        if(pesq) {
            usuariosExistentes = malloc(qtd_usuarios*sizeof(char*));
            for(int k=0;k<qtd_usuarios;k++)
                usuariosExistentes[k] = malloc(25*sizeof(char));
            int j=0;
            while((linhas=mysql_fetch_row(pesq))!=NULL) {
                for(int i=0;i<mysql_num_fields(pesq);i++) {
                    if(!i%2) {
                        sprintf(usuariosExistentes[j],"%s - ", linhas[i]);
                    } else {
                        strcat(usuariosExistentes[j], linhas[i]);
                        j++;
                    }
                }
            }
            mysql_free_result(pesq);
        }
    }
}

int retornaQtdRec() {
    int rslt;
    resultSql = mysql_query(&cnx, "select COUNT(*) from tb_records");
    if(!resultSql) {
        pesq=mysql_store_result(&cnx);
        if(pesq) {
            while((linhas=mysql_fetch_row(pesq))!=NULL) {
                rslt = atoi(linhas[0]);
            }
        }
        mysql_free_result(pesq);
    }
    return rslt;
}

void ini_mysql() {
    mysql_init(&cnx);
    mysql_real_connect(&cnx,HOST,USER,PASS,DB,0,NULL,0);
    sqlConsultUsuarios();
}

void libera_char(){
    for(int i=0;i<qtd_usuarios;i++)
        free(usuariosExistentes[i]);
    free(usuariosExistentes);
}

void fechar() {
    libera_aste();
    libera_char();
    libera_records();
    al_destroy_font(fonte24);
    al_destroy_font(fonte48);
    al_destroy_font(fonte70);
    al_destroy_font(fonte90);
    al_destroy_event_queue(fila_eventos);
    al_destroy_bitmap(nav);
    al_destroy_bitmap(prop);
    al_destroy_bitmap(tiro);
    al_destroy_display(janela);
    mysql_close(&cnx);
}

float pixels1(int angulo, float PIX) {    //Função que retorna quantos pixels devem ser somados, em relação a angulação que o bitmap (nave, asteroid, etc) se encontra, mantendo uma aceleração sempre constante em qualquer angulo.
    float pix1;
    int i;
    if((angulo%10)<5) {
        pix1=PIX;
        for(i=0;i<angulo%10;i++) {
            pix1-=((PIX-(PIX*0.7))/5);
        }
        return pix1;
    } else {
        pix1=(PIX*0.7);
        for(i=0;i<(angulo%10)-5;i++) {
            pix1-=((PIX*0.7)/5);
        }
        return pix1;
    }
}

float pixels2(int angulo, float PIX) {
    int i;
    float pix2;
    if((angulo%10)<5) {
        pix2=0;
        for(i=0;i<angulo%10;i++) {
            pix2+=((PIX*0.7)/5);
        }
        return pix2;
    } else {
        pix2=(PIX*0.7);
        for(i=0;i<(angulo%10)-5;i++) {
            pix2+=((PIX-(PIX*0.7))/5);
        }
        return pix2;
    }
}

void aloca_records() {
    int i;
    qtd_records = retornaQtdRec();
    records = malloc(qtd_records * sizeof(RECORDS*));
    for(i=0;i<qtd_records;i++)
        records[i] = malloc(sizeof(RECORDS));
}

void libera_records() {
    int i;
    for(i=0;i<qtd_records;i++) {
        free(records[i]);
    }
    free(records);
}

void aloca_aste() {
    int i;
    aste = malloc((aux.quant_aste*7) * sizeof(ASTE*));
    for(i=0;i<(aux.quant_aste*7);i++) {
        aste[i] = malloc(sizeof(ASTE));
    }
    ini_aste();
}

void libera_aste() {
    int i;
    for(i=0;i<(aux.quant_aste*7);i++) {
        free(aste[i]);
    }
    free(aste);
}

void ini_aste() {
    int i;
    for(i=0;i<aux.quant_aste;i++) {
        aste[i]->ast_pi=(rand()%TELA_ALT);
        aste[i]->ast_pj=(rand()%TELA_LARG);
        aste[i]->angulo=(rand()%40);
        aste[i]->angulo_dir=aste[i]->angulo;
        aste[i]->pixi=pixels1(aste[i]->angulo_dir, PX_ASTE);
        aste[i]->pixj=pixels2(aste[i]->angulo_dir, PX_ASTE);
        aste[i]->flag_destruido=0;
        aste[i]->livre=0;
        aste[i]->tipo=1;
    }
    for(i=aux.quant_aste;i<(aux.quant_aste*7);i++) {
        aste[i]->livre=1;
    }
}

void asteroide() {
    int i;
    for(i=0;i<(aux.quant_aste*7);i++) {
        if(!aste[i]->flag_destruido && !aste[i]->livre) {
            switch(aste[i]->angulo_dir/10) {
            case 0:
                aste[i]->ast_pi-=aste[i]->pixi;
                aste[i]->ast_pj+=aste[i]->pixj;
                break;
            case 1:
                aste[i]->ast_pj+=aste[i]->pixi;
                aste[i]->ast_pi+=aste[i]->pixj;
                break;
            case 2:
                aste[i]->ast_pi+=aste[i]->pixi;
                aste[i]->ast_pj-=aste[i]->pixj;
                break;
            case 3:
                aste[i]->ast_pj-=aste[i]->pixi;
                aste[i]->ast_pi-=aste[i]->pixj;
            }
            if(aste[i]->angulo<40) {
                aste[i]->angulo+=0.025;
            } else {
                aste[i]->angulo=0;
            }
            limite(&(aste[i]->ast_pi),&(aste[i]->ast_pj));
            switch(aste[i]->tipo) {
                case 1:
                    al_draw_rotated_bitmap(ast, R_ASTE1, R_ASTE1, aste[i]->ast_pj, aste[i]->ast_pi, aste[i]->angulo*ANG, 0);
                    break;
                case 2:
                    al_draw_rotated_bitmap(ast2, R_ASTE2, R_ASTE2, aste[i]->ast_pj, aste[i]->ast_pi, aste[i]->angulo*ANG, 0);
                    break;
                case 3:
                    al_draw_rotated_bitmap(ast3, R_ASTE3, R_ASTE3, aste[i]->ast_pj, aste[i]->ast_pi, aste[i]->angulo*ANG, 0);
            }
        }
    }
}

void movimento() {
    if(!nave.destruido) {
        if(movi.flag_acelera) {
            if(movi.acelera) {
                movi.cont_acelera=0;
                movi.acelera=0;
            }
            if(movi.cont_acelera<80) {
                if(movi.cont_acelera<20) {
                    movi.pix1=(pixels1(nave.angulo, PX_NAVE)/12)/(20-movi.cont_acelera);
                    movi.pix2=(pixels2(nave.angulo, PX_NAVE)/12)/(20-movi.cont_acelera);
                } else if(movi.cont_acelera<40) {
                    movi.pix1=((pixels1(nave.angulo, PX_NAVE)/6)/(20-(movi.cont_acelera-20)))+(pixels1(nave.angulo, PX_NAVE)/11.9);
                    movi.pix2=((pixels2(nave.angulo, PX_NAVE)/6)/(20-(movi.cont_acelera-20)))+(pixels2(nave.angulo, PX_NAVE)/11.9);
                } else if(movi.cont_acelera<60) {
                    movi.pix1=((pixels1(nave.angulo, PX_NAVE)/4)/(20-(movi.cont_acelera-40)))+(pixels1(nave.angulo, PX_NAVE)/4);
                    movi.pix2=((pixels2(nave.angulo, PX_NAVE)/4)/(20-(movi.cont_acelera-40)))+(pixels2(nave.angulo, PX_NAVE)/4);
                } else {
                    movi.pix1=((pixels1(nave.angulo, PX_NAVE)/2)/(20-(movi.cont_acelera-60)))+(pixels1(nave.angulo, PX_NAVE)/2);
                    movi.pix2=((pixels2(nave.angulo, PX_NAVE)/2)/(20-(movi.cont_acelera-60)))+(pixels2(nave.angulo, PX_NAVE)/2);
                }
                switch(nave.angulo/10) {
                case 0:
                    nave.bit_pi-=movi.pix1;
                    nave.bit_pj+=movi.pix2;
                    break;
                case 1:
                    nave.bit_pj+=movi.pix1;
                    nave.bit_pi+=movi.pix2;
                    break;
                case 2:
                    nave.bit_pi+=movi.pix1;
                    nave.bit_pj-=movi.pix2;
                    break;
                case 3:
                    nave.bit_pj-=movi.pix1;
                    nave.bit_pi-=movi.pix2;
                }
                movi.cont_acelera++;
            } else {
                movi.flag_acelera=0;
                movi.constante=1;
            }
        }
        if(movi.constante) {
            movi.pix1=pixels1(nave.angulo, PX_NAVE);
            movi.pix2=pixels2(nave.angulo, PX_NAVE);
            switch(nave.angulo/10) {
                case 0:
                    nave.bit_pi-=movi.pix1;
                    nave.bit_pj+=movi.pix2;
                    break;
                case 1:
                    nave.bit_pj+=movi.pix1;
                    nave.bit_pi+=movi.pix2;
                    break;
                case 2:
                    nave.bit_pi+=movi.pix1;
                    nave.bit_pj-=movi.pix2;
                    break;
                case 3:
                    nave.bit_pj-=movi.pix1;
                    nave.bit_pi-=movi.pix2;
                }
        }
        if(movi.flag_desacelera) {
            if(movi.desacelera) {
                movi.sub1=pixels1(nave.angulo_des, PX_NAVE);
                movi.sub2=pixels2(nave.angulo_des, PX_NAVE);
                movi.pix1=0;
                movi.pix2=0;
                if(movi.cont_acelera>80) {
                    movi.cont_desacelera=80;
                } else if(movi.cont_acelera>20) {
                    movi.cont_desacelera=movi.cont_acelera;
                } else {
                    movi.cont_desacelera=0;
                }
                movi.desacelera=0;
            }
            if(movi.cont_desacelera) {
                switch(nave.angulo_des/10) {
                    case 0:
                        nave.bit_pi-=movi.sub1;
                        nave.bit_pj+=movi.sub2;
                        break;
                    case 1:
                        nave.bit_pj+=movi.sub1;
                        nave.bit_pi+=movi.sub2;
                        break;
                    case 2:
                        nave.bit_pi+=movi.sub1;
                        nave.bit_pj-=movi.sub2;
                        break;
                    case 3:
                        nave.bit_pj-=movi.sub1;
                        nave.bit_pi-=movi.sub2;
                    }
                if(movi.cont_desacelera>60) {
                    movi.sub1=(pixels1(nave.angulo_des, PX_NAVE)/2)+((pixels1(nave.angulo_des, PX_NAVE)/40)*(movi.cont_desacelera-60));
                    movi.sub2=(pixels2(nave.angulo_des, PX_NAVE)/2)+((pixels2(nave.angulo_des, PX_NAVE)/40)*(movi.cont_desacelera-60));
                } else if(movi.cont_desacelera>40) {
                    movi.sub1=((pixels1(nave.angulo_des, PX_NAVE)/4)+((pixels1(nave.angulo_des, PX_NAVE)/80)*(movi.cont_desacelera-40)));
                    movi.sub2=((pixels2(nave.angulo_des, PX_NAVE)/4)+((pixels2(nave.angulo_des, PX_NAVE)/80)*(movi.cont_desacelera-40)));
                } else if(movi.cont_desacelera>20) {
                    movi.sub1=((pixels1(nave.angulo_des, PX_NAVE)/6)+((pixels1(nave.angulo_des, PX_NAVE)/120)*(movi.cont_desacelera-20)));
                    movi.sub2=((pixels2(nave.angulo_des, PX_NAVE)/6)+((pixels2(nave.angulo_des, PX_NAVE)/120)*(movi.cont_desacelera-20)));
                } else {
                    movi.sub1=((pixels1(nave.angulo_des, PX_NAVE)/12)+((pixels1(nave.angulo_des, PX_NAVE)/240)*(movi.cont_desacelera)));
                    movi.sub2=((pixels2(nave.angulo_des, PX_NAVE)/12)+((pixels2(nave.angulo_des, PX_NAVE)/240)*(movi.cont_desacelera)));
                }
                movi.cont_desacelera--;
            } else {
                movi.flag_desacelera=0;
            }
        }
        limite(&nave.bit_pi,&nave.bit_pj);
        if(nave.colisao) {
            colisao_nave();
        }
    }
}

void limite(float *biti, float *bitj) {
    if(*bitj>(TELA_LARG+25)) {
        *bitj=-25;
    } else if(*bitj<(-25)) {
        *bitj=(TELA_LARG+25);
    }
    if(*biti>(TELA_ALT+25)) {
        *biti=-25;
    } else if(*biti<(-26)) {
        *biti=(TELA_ALT+25);
    }
}

void disparo() {
    if(nave.flag_tiro) {
        if(!nave.intv_tiro) {
            tir0[nave.seq_tiro].angulo_tiro=nave.angulo;
            tir0[nave.seq_tiro].tiro_pi=nave.bit_pi;
            tir0[nave.seq_tiro].tiro_pj=nave.bit_pj;
            tir0[nave.seq_tiro].tiro_cont=100;
            nave.vet_tiro[nave.seq_tiro]=1;
            nave.seq_tiro++;
            if(nave.seq_tiro==6) {
                nave.seq_tiro=0;
            }
            nave.intv_tiro=25;
        } else {
            nave.intv_tiro--;
        }
    } else if(nave.intv_tiro) {
        nave.intv_tiro--;
    }
}

void acao_disparo() {
    int i;
    for(i=0;i<6;i++) {
        if(nave.vet_tiro[i]) {
            if(tir0[i].tiro_cont) {
                switch(tir0[i].angulo_tiro/10) {
                    case 0:
                        tir0[i].tiro_pi-=pixels1(tir0[i].angulo_tiro, PX_TIRO);
                        tir0[i].tiro_pj+=pixels2(tir0[i].angulo_tiro, PX_TIRO);
                        break;
                    case 1:
                        tir0[i].tiro_pj+=pixels1(tir0[i].angulo_tiro, PX_TIRO);
                        tir0[i].tiro_pi+=pixels2(tir0[i].angulo_tiro, PX_TIRO);
                        break;
                    case 2:
                        tir0[i].tiro_pi+=pixels1(tir0[i].angulo_tiro, PX_TIRO);
                        tir0[i].tiro_pj-=pixels2(tir0[i].angulo_tiro, PX_TIRO);
                        break;
                    case 3:
                        tir0[i].tiro_pj-=pixels1(tir0[i].angulo_tiro, PX_TIRO);
                        tir0[i].tiro_pi-=pixels2(tir0[i].angulo_tiro, PX_TIRO);
                }
                tir0[i].tiro_cont--;
                al_draw_rotated_bitmap(tiro, 1, 35, tir0[i].tiro_pj, tir0[i].tiro_pi, tir0[i].angulo_tiro*ANG, 0);
                colisoes_tiro(i);
            } else {
                nave.vet_tiro[i]=0;
            }
        }
    }
}

void propulcao() {
    int cy=36;
    if(movi.flag_acelera) {
        if(movi.cont_acelera==1) {
            prop=al_load_bitmap("imagens/prop1.png");
        } else if(movi.cont_acelera==40) {
            prop=al_load_bitmap("imagens/prop2.png");
        } else if(movi.cont_acelera==60) {
            prop=al_load_bitmap("imagens/prop3.png");
        }
        al_draw_rotated_bitmap(prop, 25, cy, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
    } else if(movi.constante) {
        prop=al_load_bitmap("imagens/prop4.png");
        al_draw_rotated_bitmap(prop, 25, cy, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
    } else if(movi.flag_desacelera) {
        if(movi.cont_desacelera>60) {
            prop=al_load_bitmap("imagens/prop3.png");
        } else if(movi.cont_desacelera>40) {
            prop=al_load_bitmap("imagens/prop2.png");
        } else {
            prop=al_load_bitmap("imagens/prop1.png");
        }
        al_draw_rotated_bitmap(prop, 25, cy, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
    }
}

void cria_novo_aste(int tipo_anterior, int num_anterior) {
    int i;
    if(tipo_anterior!=3) {
        for(i=0;i<2;i++) {
            if(i) {
                aste[aux.ordem_aste]->angulo=(rand()%20);
            } else {
                aste[aux.ordem_aste]->angulo=(rand()%20)+20;
            }
            aste[aux.ordem_aste]->angulo_dir=aste[aux.ordem_aste]->angulo;
            aste[aux.ordem_aste]->ast_pi=aste[num_anterior]->ast_pi;
            aste[aux.ordem_aste]->ast_pj=aste[num_anterior]->ast_pj;
            aste[aux.ordem_aste]->pixi=pixels1(aste[aux.ordem_aste]->angulo, PX_ASTE);
            aste[aux.ordem_aste]->pixj=pixels2(aste[aux.ordem_aste]->angulo, PX_ASTE);
            aste[aux.ordem_aste]->livre=0;
            aste[aux.ordem_aste]->flag_destruido=0;
            aste[aux.ordem_aste]->tipo=aste[num_anterior]->tipo+1;
            aux.ordem_aste++;
        }
    }
}

void destroi_aste(int num_tiro, int num_aste) {
    nave.vet_tiro[num_tiro]=0;
    aste[num_aste]->flag_destruido=1;
    cria_novo_aste(aste[num_aste]->tipo,num_aste);
    nave.aste_dest++;
    aux.qtd_ast_destr++;
    switch(aste[num_aste]->tipo) {
    case 1:
        aux.score+=100;
        break;
    case 2:
        aux.score+=50;
        break;
    case 3:
        aux.score+=25;
    }
    int vida_aleatoria = (rand() % 100);
    if(vida_aleatoria == 0)
        nave.vidas++;
}

void renascer_nave() {
    if(nave.vidas) {
        if(nave.destruido) {
            if(nave.cont_renascer) {
                nave.cont_renascer--;
            } else {
                nave.destruido=0;
                nave.bit_pi=TELA_ALT/2;
                nave.bit_pj=TELA_LARG/2;
                nave.colisao=0;
                nave.angulo=0;
                al_set_mouse_z(0);
                movi.flag_acelera=0;
                movi.flag_desacelera=0;
                movi.constante=0;
            }
        }
    }
}

void destroi_nave(int num_aste) {
    int i;
    nave.destruido=1;
    nave.cont_renascer=50;
    nave.vidas--;
    for(i=0;i<6;i++) {
        nave.vet_tiro[i]=0;
    }
    aste[num_aste]->flag_destruido=1;
    nave.aste_dest++;
    cria_novo_aste(aste[num_aste]->tipo,num_aste);
}

void euclidiana(float *pi1, float *pj1, float *pi2, float *pj2, int num_tiro, int num_aste, int nave) {
    float dist, cat1, cat2;
    if(*pi1>*pi1) {
        if(*pj1>*pj2) {
            cat1=*pi1-*pi2;
            cat2=*pj1-*pj2;
        } else {
            cat1=*pi1-*pi2;
            cat2=*pj2-*pj1;
        }
    } else {
        if(*pj1>*pj2) {
            cat1=*pi2-*pi1;
            cat2=*pj1-*pj2;
        } else {
            cat1=*pi2-*pi1;
            cat2=*pj2-*pj1;
        }
    }
    dist=sqrt((pow(cat1, 2)+pow(cat2, 2)));
    if(nave) {
        switch(aste[num_aste]->tipo) {
        case 1:
            if(dist<=(R_NAVE+R_ASTE1)) {
                destroi_nave(num_aste);
            }
            break;
        case 2:
            if(dist<=(R_NAVE+R_ASTE2)) {
                destroi_nave(num_aste);
            }
            break;
        case 3:
            if(dist<=(R_NAVE+R_ASTE3)) {
                destroi_nave(num_aste);
            }
        }
    } else {
        switch(aste[num_aste]->tipo) {
        case 1:
            if(dist<=(R_TIRO+R_ASTE1)) {
                destroi_aste(num_tiro, num_aste);
            }
            break;
        case 2:
            if(dist<=(R_TIRO+R_ASTE2)) {
                destroi_aste(num_tiro, num_aste);
            }
            break;
        case 3:
            if(dist<=(R_TIRO+R_ASTE3)) {
                destroi_aste(num_tiro, num_aste);
            }
        }
    }
}

void colisoes_tiro(int i) {
    int j;
    for(j=0;j<(aux.quant_aste*7);j++) {
        if(!aste[j]->flag_destruido && !aste[j]->livre) {
            euclidiana(&(aste[j]->ast_pi),&(aste[j]->ast_pj),&(tir0[i].tiro_pi),&(tir0[i].tiro_pj),i,j,0);
        }
    }
}

void grava_recorde(int atualiza) {
    if(!atualiza)
        sprintf(comando, "insert into tb_records(cod_user, stage, pontos, qtd_asteroids) values (%d, %d, %d, %d)", cod_user, aux.atual_stage, aux.score, aux.qtd_ast_destr);
    else
        sprintf(comando, "update tb_records set stage=%d, pontos=%d, qtd_asteroids=%d where cod_user=%d", aux.atual_stage, aux.score, aux.qtd_ast_destr, cod_user);
    mysql_query(&cnx,comando);
}

void testa_recorde() {
    int auxPoints;
    sprintf(comando, "select pontos from tb_records where cod_user=%d", cod_user);
    resultSql=mysql_query(&cnx, comando);
    if(!resultSql) {
        pesq=mysql_store_result(&cnx);
        int flag = mysql_num_rows(pesq);
        if(!flag)
            grava_recorde(0);
        else {
            if(pesq) {
                while((linhas=mysql_fetch_row(pesq))!=NULL) {
                    auxPoints = atoi(linhas[0]);
                }
                mysql_free_result(pesq);
            }
            if(auxPoints<aux.score)
                grava_recorde(1);
        }
    }
}

void colisao_nave() {
    int j;
    if(!nave.destruido && nave.vidas) {
        for(j=0;j<(aux.quant_aste*7);j++) {
            if(!aste[j]->flag_destruido && !aste[j]->livre) {
                euclidiana(&(aste[j]->ast_pi),&(aste[j]->ast_pj),&(nave.bit_pi),&(nave.bit_pj),-1,j,1);
            }
        }
    }
}

void game_over() {
    if(aux.cont_game_over) {
        if(aux.cont_game_over==5)
            testa_recorde();
        aux.cont_game_over--;
    } else {
        al_draw_bitmap(fnd_ini,0,0,0);
        al_draw_text(fonte90, al_map_rgb(255,255,255), TELA_LARG*0.5, TELA_ALT*0.4, ALLEGRO_ALIGN_CENTER, "GAME OVER");
        al_draw_text(fonte24, al_map_rgb(255,255,255), TELA_LARG*0.5, TELA_ALT*0.6, ALLEGRO_ALIGN_CENTER, "*Sair - (Esc)*");
    }
}

void sob_tela() {
    char estagio[12],score[20];
    sprintf(estagio, "STAGE: %d", aux.atual_stage);
    al_draw_text(fonte24,al_map_rgb(222,222,222),TELA_LARG*0.2,TELA_ALT*0.05,ALLEGRO_ALIGN_RIGHT,estagio);
    al_draw_text(fonte24,al_map_rgb(222,222,222),TELA_LARG*0.75,TELA_ALT*0.05,ALLEGRO_ALIGN_CENTRE,"Vidas: ");
    for(int i=1;i<=nave.vidas;i++) {
        al_draw_bitmap(vida,(TELA_LARG*0.76)+(i*50),(TELA_ALT*0.05),0);
    }
    if(!aux.score) {
        sprintf(score, "SCORE: 000000000");
    } else if(aux.score<1000) {
        sprintf(score, "SCORE: 000000%d", aux.score);
    } else if(aux.score<10000) {
        sprintf(score, "SCORE: 00000%d", aux.score);
    } else if(aux.score<100000) {
        sprintf(score, "SCORE: 0000%d", aux.score);
    } else if(aux.score<1000000) {
        sprintf(score, "SCORE: 000%d", aux.score);
    } else if(aux.score<10000000) {
        sprintf(score, "SCORE: 00%d", aux.score);
    } else if(aux.score<100000000) {
        sprintf(score, "SCORE: 0%d", aux.score);
    } else {
        sprintf(score, "SCORE: %d", aux.score);
    }
    al_draw_text(fonte24, al_map_rgb(222, 222, 222),(TELA_LARG*0.18),(TELA_ALT*0.92),ALLEGRO_ALIGN_CENTER,score);
}

void stage_pass() {
    char stg[10];
    sprintf(stg, "STAGE %d", aux.atual_stage);
    al_draw_text(fonte48, al_map_rgb(222,222,222),TELA_LARG/2,TELA_ALT/4,ALLEGRO_ALIGN_CENTRE,stg);
}

void desenha() {
    al_clear_to_color(al_map_rgb(0,0,0));
    if(!nave.destruido) {
        al_draw_rotated_bitmap(nav, 25, R_NAVE, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
        propulcao();
        acao_disparo();
    }
    asteroide();
    sob_tela();
    if(!nave.vidas) {
        game_over();
    }
    if(nave.cont_pass_level<200) {
        al_draw_text(fonte70, al_map_rgb(222,222,222), TELA_LARG*0.5, TELA_ALT*0.4, ALLEGRO_ALIGN_CENTRE, "LEVEL CLEAR!");
    }
    if(aux.cont_stage) {
        stage_pass();
        aux.cont_stage--;
    }
    al_flip_display();
    al_rest(FPS);
}

void direcionais_aux() {
    if(aux.flag_direita) {
        if(!aux.cont_direcional) {
            if(nave.angulo<39) {
                nave.angulo++;
            } else {
                nave.angulo=0;
            }
            al_set_mouse_z(nave.angulo);
            aux.cont_direcional=1;
        } else {
            aux.cont_direcional--;
        }
    }
    if(aux.flag_esquerda) {
        if(!aux.cont_direcional) {
            if(nave.angulo>=0) {
                nave.angulo--;
            } else {
                nave.angulo=39;
            }
            al_set_mouse_z(nave.angulo);
            aux.cont_direcional=1;
        } else {
            aux.cont_direcional--;
        }
    }
}

void procedimentos() {
    direcionais_aux();
    movimento();
    disparo();
    desenha();
    renascer_nave();
    test_nivel();
}

void passa_nivel() {
    int i;
    if(nave.cont_pass_level) {
        nave.cont_pass_level--;
    } else {
        libera_aste();
        al_rest(0.1);
        aux.atual_stage++;
        aux.quant_aste+=2;
        aux.ordem_aste=aux.quant_aste;
        aux.cont_stage=150;
        aloca_aste();
        nave.bit_pi=TELA_ALT/2;
        nave.bit_pj=TELA_LARG/2;
        nave.colisao=0;
        nave.angulo=0;
        nave.aste_dest=0;
        al_set_mouse_z(0);
        movi.flag_acelera=0;
        movi.flag_desacelera=0;
        movi.constante=0;
        nave.cont_pass_level=200;
        for(i=0;i<6;i++) {
            nave.vet_tiro[i]=0;
        }
    }
}

void test_nivel() {
    if(nave.aste_dest==(aux.quant_aste*7)) {
        passa_nivel();
    }
}

void jogo() {
    int tecla=0;
    while(!al_is_event_queue_empty(fila_eventos)) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);
        switch(evento.type) {
            case ALLEGRO_EVENT_MOUSE_AXES:
                if(evento.mouse.z<0) {
                    al_set_mouse_z(39);
                } else if(evento.mouse.z>39) {
                    al_set_mouse_z(0);
                }
                nave.angulo=evento.mouse.z;
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                if(evento.mouse.button==2 && nave.colisao) {
                    nave.flag_tiro=1;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                if(evento.mouse.button==2 && nave.colisao) {
                    nave.flag_tiro=0;
                }
                break;
            case ALLEGRO_EVENT_KEY_DOWN:
                switch(evento.keyboard.keycode) {
                    case ALLEGRO_KEY_SPACE:
                        if(!nave.colisao) {
                            nave.colisao=1;
                        }
                        if(!movi.flag_acelera && !movi.constante) {
                            movi.flag_acelera=1;
                            movi.acelera=1;
                        }
                        break;
                    case ALLEGRO_KEY_A:
                    case ALLEGRO_KEY_LEFT:
                        aux.flag_esquerda=1;
                        break;
                    case ALLEGRO_KEY_D:
                    case ALLEGRO_KEY_RIGHT:
                        aux.flag_direita=1;
                        break;
                    case ALLEGRO_KEY_LSHIFT:
                        if(nave.colisao) {
                            nave.flag_tiro=1;
                        }
                        break;
                    case ALLEGRO_KEY_ESCAPE:
                        if(nave.vidas) {
                            al_draw_bitmap(fnd_ini,0,0,0);
                            al_draw_text(fonte70, al_map_rgb(222,222,222), TELA_LARG*0.5, TELA_ALT*0.4, ALLEGRO_ALIGN_CENTER, "DESEJA SAIR? (Y/N)");
                            al_flip_display();
                            do {
                                ALLEGRO_EVENT evento2;
                                al_wait_for_event(fila_eventos, &evento2);
                                if(evento.type==ALLEGRO_EVENT_KEY_DOWN) {
                                    switch(evento2.keyboard.keycode) {
                                    case ALLEGRO_KEY_Y:
                                        tecla=1;
                                        break;
                                    case ALLEGRO_KEY_N:
                                        tecla=2;
                                    }
                                }
                            }while(!tecla);
                            if(tecla==1) {
                                libera_aste();
                                ini_parametros();
                            }
                        } else {
                            libera_aste();
                            ini_parametros();
                        }
                }
                break;
            case ALLEGRO_EVENT_KEY_UP:
                switch(evento.keyboard.keycode) {
                    case ALLEGRO_KEY_A:
                    case ALLEGRO_KEY_LEFT:
                        aux.flag_esquerda=0;
                        break;
                    case ALLEGRO_KEY_D:
                    case ALLEGRO_KEY_RIGHT:
                        aux.flag_direita=0;
                        break;
                    case ALLEGRO_KEY_LSHIFT:
                        if(nave.colisao) {
                            nave.flag_tiro=0;
                        }
                        break;
                    case ALLEGRO_KEY_SPACE:
                        if(movi.flag_acelera) {
                            movi.flag_acelera=0;
                            if(!movi.flag_desacelera) {
                                movi.flag_desacelera=1;
                                movi.desacelera=1;
                                nave.angulo_des=nave.angulo;
                            }
                        }
                        if(movi.constante) {
                            movi.flag_desacelera=1;
                            movi.desacelera=1;
                            movi.constante=0;
                            nave.angulo_des=nave.angulo;
                        }
                }
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                fechar();
                exit(1);
        }
    }
}

void consultaRecords() {
    int i=0;
    resultSql = mysql_query(&cnx, "select us.nome, rec.stage, rec.pontos, rec.qtd_asteroids from tb_user as us, tb_records as rec where us.cod_user=rec.cod_user order by pontos desc");
    if(!resultSql) {
        pesq=mysql_store_result(&cnx);
        if(pesq) {
            while((linhas=mysql_fetch_row(pesq))!=NULL) {
                sprintf(records[i]->nome, "%s", linhas[0]);
                sprintf(records[i]->stage, "%s", linhas[1]);
                sprintf(records[i]->pontos, "%s", linhas[2]);
                sprintf(records[i]->qtd_asteroids, "%s", linhas[3]);
                i++;
            }
            mysql_free_result(pesq);
        }
    }
}

void tela_inicial() {
    al_draw_bitmap(fnd_ini,0,0,0);
    al_draw_rotated_bitmap(logo,424,89,TELA_LARG/2,TELA_ALT*0.2,0,0);
    switch(tec_inicial) {
        case 1:
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Jogar");///
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Instrucoes");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Recordes");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.7,ALLEGRO_ALIGN_CENTRE,"Sair");
            break;
        case 2:
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Jogar");
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Instrucoes");///
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Recordes");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.7,ALLEGRO_ALIGN_CENTRE,"Sair");
            break;
        case 3:
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Jogar");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Instrucoes");
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Recordes");///
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.7,ALLEGRO_ALIGN_CENTRE,"Sair");
            break;
        case 4:
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Jogar");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Instrucoes");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Recordes");
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.7,ALLEGRO_ALIGN_CENTRE,"Sair");///
    }
    while(!al_is_event_queue_empty(fila_eventos)) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);
        switch(evento.type) {
        case ALLEGRO_EVENT_KEY_CHAR:
            switch(evento.keyboard.keycode) {
            case ALLEGRO_KEY_W:
            case ALLEGRO_KEY_UP:
                if(tec_inicial>1) {
                    tec_inicial--;
                }
                break;
            case ALLEGRO_KEY_S:
            case ALLEGRO_KEY_DOWN:
                if(tec_inicial<4) {
                    tec_inicial++;
                }
                break;
            case ALLEGRO_KEY_ESCAPE:
                fechar();
                exit(1);
                break;
            case ALLEGRO_KEY_ENTER:
                switch(tec_inicial) {
                case 1:
                    opc_menu=1;
                    break;
                case 2:
                    opc_menu=2;
                    break;
                case 3:
                    if(!flag_records) {
                        aloca_records();
                        consultaRecords();
                        flag_records = 1;
                    } else {
                        libera_records();
                        aloca_records();
                        consultaRecords();
                    }
                    opc_menu=3;
                    break;
                case 4:
                    fechar();
                    exit(1);
                }
            }
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            fechar();
            exit(1);
        }
    }
    al_flip_display();
    al_rest(FPS);
}

int instrucoes() {
    al_draw_bitmap(fnd_ini,0,0,0);
    al_draw_text(fonte70,al_map_rgb(222,222,222),TELA_LARG/2,TELA_ALT*0.15,ALLEGRO_ALIGN_CENTRE,"Movimentos:");
    al_draw_text(fonte48,al_map_rgb(222,222,222),TELA_LARG/2,TELA_ALT*0.25,ALLEGRO_ALIGN_CENTRE,"Setas esquerda e direita ou bolinha do mouse");
    al_draw_text(fonte70,al_map_rgb(222,222,222),TELA_LARG/2,TELA_ALT*0.45,ALLEGRO_ALIGN_CENTRE,"Acao:");
    al_draw_text(fonte48,al_map_rgb(222,222,222),TELA_LARG/2,TELA_ALT*0.55,ALLEGRO_ALIGN_CENTRE,"Espaco acelera, shift ou mouse direito dispara");
    al_flip_display();
    while(!al_is_event_queue_empty(fila_eventos)) {
        ALLEGRO_EVENT evento3;
        al_wait_for_event(fila_eventos, &evento3);
        if(evento3.type==ALLEGRO_EVENT_KEY_DOWN) {
            if(evento3.keyboard.keycode==ALLEGRO_KEY_SPACE || evento3.keyboard.keycode==ALLEGRO_KEY_ESCAPE) {
                return 0;
            }
        }
    }
    return 2;
}

void loginUser() {
    al_draw_bitmap(fnd_ini,0,0,0);
    al_draw_rotated_bitmap(logo,424,89,TELA_LARG/2,TELA_ALT*0.2,0,0);
    switch(tec_inicial_user) {
        case 1:
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Criar Usuario");///
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Usuario Existente");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Sair");
            break;
        case 2:
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Criar Usuario");
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Usuario Existente");///
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Sair");
            break;
        case 3:
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.4,ALLEGRO_ALIGN_CENTRE,"Criar Usuario");
            al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.5,ALLEGRO_ALIGN_CENTRE,"Usuario Existente");
            al_draw_text(fonte48, al_map_rgb(255,36,36),TELA_LARG/2,TELA_ALT*0.6,ALLEGRO_ALIGN_CENTRE,"Sair");///
    }
    while(!al_is_event_queue_empty(fila_eventos)) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);
        switch(evento.type) {
            case ALLEGRO_EVENT_KEY_CHAR:
                switch(evento.keyboard.keycode) {
                case ALLEGRO_KEY_W:
                case ALLEGRO_KEY_UP:
                    if(tec_inicial_user>1) {
                        tec_inicial_user--;
                    }
                    break;
                case ALLEGRO_KEY_S:
                case ALLEGRO_KEY_DOWN:
                    if(tec_inicial_user<3) {
                        tec_inicial_user++;
                    }
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    fechar();
                    exit(1);
                    break;
                case ALLEGRO_KEY_ENTER:
                    switch(tec_inicial_user) {
                        case 1:
                            opc_user=1;
                            break;
                        case 2:
                            opc_user=2;
                            break;
                        case 3:
                            fechar();
                            exit(1);
                    }
                }
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                fechar();
                exit(1);
        }
    }
    al_flip_display();
    al_rest(FPS);
}

void criarUser() {
    al_draw_bitmap(fnd_ini,0,0,0);
    al_draw_rotated_bitmap(logo,424,89,TELA_LARG/2,TELA_ALT*0.2,0,0);
    al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG*0.4,TELA_ALT*0.4, ALLEGRO_ALIGN_CENTRE,"Nome:");
    al_draw_textf(fonte48, al_map_rgb(255,237,234),TELA_LARG*0.45, TELA_ALT*0.4, 0, "%s", nome_user);
    while(!al_is_event_queue_empty(fila_eventos)){
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);
        if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            int tec=evento.keyboard.keycode;
            if(tec>0 && tec<27) {// Letras de a..z
                let[0]=96+tec;
                let[1]='\0';
                if(count_criar_user<14) {
                    nome_user[count_criar_user]=let[0];
                    nome_user[count_criar_user+1]='\0';
                    count_criar_user++;
                }
            } else if(tec==63 && count_criar_user>0) {//Backspace
                count_criar_user--;
                nome_user[count_criar_user]='\0';
            } else if(tec==59) {//Escape
                fechar();
                exit(1);
            } else if(tec==67) {//Enter
                sprintf(comando, "insert into tb_user(nome) values ('%s')", nome_user);
                resultSql = mysql_query(&cnx, comando);
                if(!resultSql) {
                    sprintf(comando, "select tb_user.cod_user from tb_user where tb_user.nome='%s'", nome_user);
                    resultSql=mysql_query(&cnx, comando);
                    if(!resultSql) {
                        pesq = mysql_store_result(&cnx);
                        if(pesq) {
                            while((linhas=mysql_fetch_row(pesq))!=NULL) {
                                cod_user=atoi(linhas[0]);
                            }
                        }
                    }
                }
            }
        }
    }
    al_flip_display();
    al_rest(FPS);
}

void existeUser() {
    al_draw_bitmap(fnd_ini,0,0,0);
    al_draw_rotated_bitmap(logo,424,89,TELA_LARG/2,TELA_ALT*0.2,0,0);
    if(qtd_usuarios) {
        float positionY = 0.4;
        al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT*0.3,ALLEGRO_ALIGN_CENTRE,"Digite o codigo do jogador:");
        for(int i=0;i<qtd_usuarios;i++) {
            al_draw_textf(fonte48, al_map_rgb(255,237,234),TELA_LARG/2, TELA_ALT*positionY, ALLEGRO_ALIGN_CENTRE, "%s", usuariosExistentes[i]);
            positionY += 0.07;
        }
    } else
        al_draw_text(fonte48, al_map_rgb(255,237,234),TELA_LARG/2,TELA_ALT/2,ALLEGRO_ALIGN_CENTRE,"Nenhum usuario cadastrado!");
    while(!al_is_event_queue_empty(fila_eventos)) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);
        switch(evento.type) {
            case ALLEGRO_EVENT_KEY_CHAR:
                if(evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    fechar();
                    exit(1);
                }
                if(qtd_usuarios)
                    if((evento.keyboard.keycode>27 && evento.keyboard.keycode<=(27+qtd_usuarios)) || (evento.keyboard.keycode>37 && evento.keyboard.keycode<=(37+qtd_usuarios)))
                        if(evento.keyboard.keycode>27 && evento.keyboard.keycode<=(27+qtd_usuarios))
                            cod_user = evento.keyboard.keycode - 27;
                        else
                            cod_user = evento.keyboard.keycode - 37;
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                fechar();
                exit(1);
        }
    }
    al_flip_display();
    al_rest(FPS);
}

int recordes() {
    al_draw_bitmap(fnd_ini,0,0,0);
    al_draw_text(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.6,TELA_ALT*0.07,ALLEGRO_ALIGN_CENTRE,"Nome      / Pontos / Stage / Destr.");
    al_draw_text(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.18,TELA_ALT*0.07,ALLEGRO_ALIGN_CENTRE,"#.");
    for(int i=1;i<=qtd_records;i++) {
        al_draw_text(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.6,TELA_ALT*(0.1+(i*0.07)),ALLEGRO_ALIGN_CENTRE,"          /        /       /       ");
        al_draw_textf(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.18,TELA_ALT*(0.1+(i*0.07)),ALLEGRO_ALIGN_CENTRE,"%d.", i);
        al_draw_textf(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.33,TELA_ALT*(0.1+(i*0.07)),ALLEGRO_ALIGN_CENTRE,"%s", records[i-1]->nome);
        al_draw_textf(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.56,TELA_ALT*(0.1+(i*0.07)),ALLEGRO_ALIGN_CENTRE,"%s", records[i-1]->pontos);
        al_draw_textf(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.71,TELA_ALT*(0.1+(i*0.07)),ALLEGRO_ALIGN_CENTRE,"%s", records[i-1]->stage);
        al_draw_textf(fonte48,al_map_rgb(255,237,234),TELA_LARG*0.85,TELA_ALT*(0.1+(i*0.07)),ALLEGRO_ALIGN_CENTRE,"%s", records[i-1]->qtd_asteroids);
    }
    al_flip_display();
    while(!al_is_event_queue_empty(fila_eventos)) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);
        if(evento.type==ALLEGRO_EVENT_KEY_DOWN) {
            if(evento.keyboard.keycode==ALLEGRO_KEY_SPACE || evento.keyboard.keycode==ALLEGRO_KEY_ESCAPE) {
                return 0;
            }
        }
    }
    return 3;
}

int menu() {
    switch(opc_menu) {
        case 0:
            al_clear_to_color(al_map_rgb(0,0,0));
            al_draw_rotated_bitmap(nav, 25, 25, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
            asteroide();
            if(cod_user == -1) {
                switch(opc_user) {
                    case 0:
                        loginUser();
                        break;
                    case 1:
                        criarUser();
                        break;
                    case 2:
                        existeUser();
                }
            } else
                tela_inicial();
            break;
        case 1:
            procedimentos();
            jogo();
            break;
        case 2:
            al_clear_to_color(al_map_rgb(0,0,0));
            al_draw_rotated_bitmap(nav, 25, 25, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
            asteroide();
            opc_menu = instrucoes();
            al_rest(FPS);
            break;
        case 3:
            al_clear_to_color(al_map_rgb(0,0,0));
            al_draw_rotated_bitmap(nav, 25, 25, nave.bit_pj, nave.bit_pi, nave.angulo*ANG, 0);
            asteroide();
            opc_menu = recordes();
            al_rest(FPS);
    }
    return 1;
}

int main(void) {
    if (!ini_alle()){
        return -1;
    } else {
        ini_mysql();
        ini_parametros();
    }
    while(menu());
}
