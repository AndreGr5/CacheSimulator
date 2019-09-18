#include <bits/stdc++.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include <locale.h>
#include <windows.h>

#define tamMP 64 ///Tamanho da Memoria Principal; * ( N )
#define END 6 ///Tamanho do enderaçamento da memória; * ( E )
#define tamBloco 4 ///Quantidade de blocos na memória cache; * ( X )
#define MCACHE 4 ///Quantidade de linhas da memoria cache; * ( L )
#define mbits 4/// tamanho de bits do endereço de bloco da memória; *

//#define BIT 10 ///Tamanho máximo da instrução;

#define t 3 /// tamanho da tag; *
#define ind 1 /// tamanho do indice; *
#define offs 2 /// tamanho do offset; *

#define indmd 2 /// tamanho do indice mapeado diretamente *
#define tmd 2 /// tamanho da tag mapeada diretamente *
#define offsmd 2 /// tamanho do offset mapeado diretamente *


#define qtdLinhas 2 /// Quantidade de linhas por conjunto; *
#define qtdConjuntos 2 /// Quantidade de conjuntos; *

using namespace std;

/*-----------------Cores do DOS-------------------------------------------------
Descrição: Procedimento para definição de Cores do DOS
------------------------------------------------------------------------------*/
enum DOS_COLORS {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN,
    LIGHT_GRAY, DARK_GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN,
    LIGHT_RED, LIGHT_MAGENTA, YELLOW, WHITE };
/*----------------------------------------------------------------------------*/

/*-----------------Cores das letras---------------------------------------------
Descrição: Procedimento para inserir cores de letras no programa
------------------------------------------------------------------------------*/
void textcolor (DOS_COLORS iColor)
{
    HANDLE hl = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    BOOL b = GetConsoleScreenBufferInfo(hl, &bufferInfo);
    bufferInfo.wAttributes &= 0x00F0;
    SetConsoleTextAttribute (hl, bufferInfo.wAttributes |= iColor);
}
/*----------------------------------------------------------------------------*/

void gotoxy(int x, int y){
     SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),(COORD){x-1,y-1});
}



/// Estruturas//////////////////////////////////////////////////////

typedef struct Linha{
    //int bitV; //?
    int tag=-1;
    int lfu=0;  /// para substituição da linha;
    char bloco[tamBloco][15];
    int local=-1;

}linha;

typedef struct Conjunto{
    linha lcache[qtdLinhas];
}conjunto;

typedef struct EnderecoMD{
    int tag;
    int indice;
    int offset;
    int* binario;
}adressMD;

typedef struct EnderecoAC{
    int tag;
    int conjunto;
    int byte;
    int* binario;
}adressAC;

typedef struct EnderecoTA{
    int tag;
    int byte;
    int* binario;
}adressTA;

typedef struct MemoriaPrincipal{
    char conteudo[10];
    int bloco;
}memoriaP;

/// Variáveis Globais/////////////////////////////////////////////////

linha cacheMD[MCACHE]; /// Cache MD e Totalmente assossiativa
conjunto Ccache[qtdConjuntos]; /// Cache Assossiativa por conjunto
memoriaP mp[tamMP];

FILE *memoria; /// arquivo com as instruções

int conjuntoCheio[qtdConjuntos];
int cacheCheia=0; /// TA

queue<int> fila[qtdConjuntos];

void printaCache(int tipo);
void printaLinha(int L,int C);

adressMD TelaMD;
int hitMD=-1;
int hitAC=-1;
int hitTA=-1;
int line =-1;

/// //////////////////////////////////////////////////////////////////

int GeraArquivo(){
    FILE *arquivo = fopen("entrada.txt","w");

    for(int i=0;i<tamMP+20;i++){
        fprintf(arquivo,"0x1");
        fprintf(arquivo,"%.7d",i);
    }
    fclose(arquivo);

    FILE *sobre = fopen("sobre.txt","w");

    fprintf(sobre,"Feito por André Gomes\n\nContato: andregr@id.uff.br\n\nEsse programa faz parte do projeto de monitoria da disciplina Fundamentos de Arquitetura de Computadores da UFF de Rio das Ostras");
    fclose(sobre);

}
void Restore(){
    cacheCheia=0;
    memset(conjuntoCheio,0,sizeof(int)*qtdConjuntos);
    hitMD=-1;
    hitAC=-1;

    for(int i=0;i<MCACHE;i++){
        conjuntoCheio[i]=0;
        cacheMD[i].lfu=0;
        cacheMD[i].tag=-1;
        for(int k=0;k<tamBloco;k++){
            strcpy(cacheMD[i].bloco[k],"xxxxxxxxxx");
        }
    }

    for(int i=0;i<qtdConjuntos;i++){
            for(int j=0;j<qtdLinhas;j++){
                Ccache[i].lcache[j].lfu=0;
                Ccache[i].lcache[j].tag=-1;
                for(int k=0;k<tamBloco;k++){
                    strcpy(Ccache[i].lcache[j].bloco[k],"xxxxxxxxxx");
                }
            }
    }
}

/// Funçoes de Manipulação de números ////////////////////////////////

int pot(int a, int b){
    if(b==0) return 1;
    int resul= a;
    for(int i=1;i<b;i++){
        resul*=a;
    }
    return resul;
}

int bin_to_dec(int bin){
    int total=0;
    int potenc=1;

    while(bin>0){
        total+=bin%10*potenc;
        bin = bin/10;
        potenc = potenc*2;
    }
    return total;
}

int dec_to_bin(int k){
    if ( k < 2 )
        return k;
    return ( 10 * dec_to_bin( k / 2 ) ) + k % 2;
}

int vetor_to_int(int *vet, int a, int b){
    int resul=0;
    for(int i=a;i<=b;i++){
        resul+=vet[i]*pot(10,(b)-i);
    }
    return resul;
}

int vetor_to_int(char *vet, int a, int b){
    int resul=0;
    int numero;
    for(int i=a;i<=b;i++){
        numero=vet[i]-'0';
        resul+=numero*pot(10,(5-1)-i);
    }
    return resul;
}

int* dec_to_vet(int dec, int tam){
    int *vetor;
    int limite=pot(10,tam);
    vetor=(int *) malloc(tam*sizeof(int));

    for(int i=0;i<tam;i++){
        limite/=10;
        vetor[i]=(dec/limite)%10;
    }
    return vetor;
}

/// Funções da Cache//////////////////////////////////////////////////////

void IniciaMemoria(){ /// Pega o arquivo de texto e coloca no vetor memoria;
    char instrucao[11];
    memoria = fopen("entrada.txt","r");
    int cont=0;

    for(int i=0;i<tamMP;i++){
        fgets(instrucao,11,memoria);
        strcpy(mp[cont].conteudo,instrucao);
        mp[cont].bloco=cont/tamBloco;
        //cout << cont/tamBloco << " recebe: " << instrucao << endl;
        cont++;
    }
    //cout<<"Memoria Carregada\n"<<endl;
    fclose(memoria);

    memset(conjuntoCheio,0,sizeof(int)*qtdConjuntos);

}

int procuraCache(adressAC endereco){ /// Procura a instrução na memória cache;
    for(int i=0;i<qtdLinhas;i++){
        if(Ccache[(endereco.conjunto)].lcache[i].local==-1) Ccache[(endereco.conjunto)].lcache[i].local=i;
        if(Ccache[endereco.conjunto].lcache[i].tag==endereco.tag){
            line=Ccache[endereco.conjunto].lcache[i].local;
            return 1;
        }
    }
    return 0;
}

int procuraCache(adressMD endereco){ /// Procura a instrução na memória cache;
    if(cacheMD[endereco.indice].tag == endereco.tag) return 1; /// Cache hit;
        else return 0;
}

int procuraCache(adressTA endereco){ /// Procura a instrução na memória cache;
    for(int i=0;i<MCACHE;i++)
        if(cacheMD[i].tag == endereco.tag){
            cacheMD[i].lfu++;
            line=cacheMD[i].local;
            return 1; /// Cache hit;
        }
    return 0;
}


void gravaCache(adressMD endereco){ //Acesso Direto
    linha aux;

    int endBinario = vetor_to_int(endereco.binario,0,mbits-1); /// pega os n bits mais significativos que representam o endereco do bloco da memoria;
    //cout<< "Endereco binario do bloco: "<< endBinario<<" ";
    endBinario=bin_to_dec(endBinario); /// transforma esse endereço e transforma em decimal;
    endBinario*= tamBloco; /// multiplica pelo tamanho do bloco para ver em qual parte do vetor da MP ele está localizado;
    //cout <<"Endereco decimal da MP:" << endBinario<<endl;

    for(int i=0;i<tamBloco;i++){
        strcpy(aux.bloco[i],mp[endBinario+i].conteudo);
        //cout<<aux.bloco[i];
    }

    aux.tag=endereco.tag;
    cacheMD[endereco.indice]=aux;

}

int menorLFU(linha* linhas, int tam){
    int menor = 99999;
    int menorID;

    for(int i=0;i<tam;i++){
        if(linhas[i].lfu<menor){
            menor=linhas[i].lfu;
            menorID=i;
        }
    }

    return menorID;
}

int LFUiguais(linha* linhas, int tam){
    for(int i=1;i<tam;i++){
        if(linhas[i].lfu!=linhas[i-1].lfu)
            return 0;
    }

    return 1;
}
void gravaCache(adressTA endereco, int modo){ // Totalmente Associativa
    linha aux;
    srand(time(NULL));
    //int line;

    int endBinario = vetor_to_int(endereco.binario,0,mbits-1); /// pega os n bits mais significativos que representam o endereco do bloco da memoria;
    endBinario=bin_to_dec(endBinario); /// transforma esse endereço e transforma em decimal;
    endBinario*= tamBloco; /// multiplica pelo tamanho do bloco para ver em qual parte do vetor da MP ele está localizado;

    for(int i=0;i<tamBloco;i++){
        strcpy(aux.bloco[i],mp[endBinario+i].conteudo);
        //cout<<aux.bloco[i];
    }
    aux.tag=endereco.tag;

    if(cacheCheia<MCACHE){
        line=cacheCheia;
        cacheCheia++;
    }else{
        switch(modo){
            case 0:{ /// Acesso aleatório;
                line=rand()%MCACHE;
                //cout <<"Cache cheia! Copiado para a linha "<<line<<" por acesso aleatorio"<<endl;
                break;
            }
            case 1:{ /// FIFO
                line=cacheCheia%MCACHE;
                cacheCheia++;
                //cout <<"Cache cheia! Copiado para a linha "<<line<<" por acesso FIFO"<<endl;
                break;
            }
            case 2:{ /// LFU
                line=menorLFU(cacheMD,MCACHE);
                //cout <<"Cache cheia! Copiado para a linha "<<line<<" por acesso LFU"<<endl;
                break;
            }
        }
    }
    cacheMD[line]=aux;
    cacheMD[line].lfu++;
    for(int i=0;i<MCACHE;i++){
        //cout << i <<" : "<< cacheMD[i].lfu << "\n";
        if(i!=line && cacheMD[i].lfu>0){
            cacheMD[i].lfu--;
        }
    }
    //cout<< endl;
    cacheMD[line].local=line;
    cacheMD[line].lfu++;
}

void gravaCache(adressAC endereco, int modo){
    linha aux;
    int con=endereco.conjunto;
    srand(time(NULL));
    //int line;

    int endBinario = vetor_to_int(endereco.binario,0,mbits-1); /// pega os n bits mais significativos que representam o endereco do bloco da memoria;
    //cout<< "Endereco binario do bloco: "<< endBinario<<" ";
    endBinario=bin_to_dec(endBinario); /// transforma esse endereço e transforma em decimal;
    endBinario*= tamBloco; /// multiplica pelo tamanho do bloco para ver em qual parte do vetor da MP ele está localizado;
    //cout <<"| Endereco decimal da MP: " << endBinario<<endl;

    for(int i=0;i<tamBloco;i++){
        strcpy(aux.bloco[i],mp[endBinario+i].conteudo);
        //cout<<aux.bloco[i];
    }

    aux.tag=endereco.tag;

    if(conjuntoCheio[(endereco.conjunto)]<qtdLinhas){
        line=conjuntoCheio[(endereco.conjunto)];
        conjuntoCheio[endereco.conjunto]++;
    }else{
        switch(modo){
            case 0:{ /// Acesso aleatório;
                line=rand()%qtdLinhas;
                //cout <<"Cache cheia! Copiado para a linha "<<line<<" do conjunto "<<endereco.conjunto<< " por acesso aleatorio"<<endl;
                break;

            }

            case 1:{ /// FIFO
                line=conjuntoCheio[endereco.conjunto]%qtdLinhas;
                conjuntoCheio[endereco.conjunto]++;
                //cout <<"Cache cheia! Copiado para a linha "<<line<<" do conjunto "<<endereco.conjunto<< " por acesso FIFO"<<endl;
                break;
            }
        }
    }

    Ccache[(endereco.conjunto)].lcache[line]=aux;
    Ccache[(endereco.conjunto)].lcache[line].local=line;
}

void ciclo(int binario, int tipo){

    switch(tipo){
        case (0):{ /// Mapeada Diretamente
            adressMD aux;
            int* vetorBinario;

            ///montagem do endereco///
            vetorBinario=dec_to_vet(binario,END);
            aux.binario=vetorBinario;

            /*for(int i=0;i<END;i++){
                cout << aux.binario[i];
                if(i==(tmd-1) || i==(tmd+indmd-1)) cout << " ";
            }*/

            //cout << endl;
            aux.tag=bin_to_dec(vetor_to_int(vetorBinario,0,tmd-1));
            aux.indice=bin_to_dec(vetor_to_int(vetorBinario,tmd,tmd+indmd-1));
            aux.offset=bin_to_dec(vetor_to_int(vetorBinario,tmd+indmd,tmd+indmd+offsmd-1));
            //cout << "Tag: "<< aux.tag << " Linha: "<<aux.indice<<" Offset: "<<aux.offset<<endl;
            ///                   ///
            //system("pause");
            TelaMD=aux;

            if (procuraCache(aux)){
                //cout <<"Cache hit"<<endl;
                hitMD=1;
            }else{
                //cout <<"Cache miss!"<<endl;
                hitMD=0;
                gravaCache(aux);
            }

            break;
        }

        case (1):{ /// Totalmente Associativa
            adressTA aux;
            int* vetorBinario;

            ///montagem do endereco///
            vetorBinario=dec_to_vet(binario,END);
            aux.binario=vetorBinario;

            /*for(int i=0;i<END;i++){
                cout << aux.binario[i];
                if(i==(MCACHE-1)) cout << " ";

            }*/
            //cout << endl;

            aux.tag=bin_to_dec(vetor_to_int(vetorBinario,0,mbits-1));
            aux.byte=bin_to_dec(vetor_to_int(vetorBinario,mbits,mbits+offs-1));
            //cout << "Tag: "<< aux.tag <<" Byte: "<<aux.byte<<endl;
            ///                   ///

            if (procuraCache(aux)){
                //cout <<"Cache hit"<<endl;
                //for(int i=0;i<MCACHE;i++) /// mostrar linhas e lfu
                    //cout << i <<" : "<< cacheMD[i].lfu << "\n";
                hitTA=1;
            }else{
                //cout <<"Cache miss!"<<endl;
                hitTA=0;
                gravaCache(aux,1);
            }

            break;
        }

        case (2):{ /// Associativa por conjunto
            adressAC aux;
            int* vetorBinario;
            int modo=1;

            ///montagem do endereco///
            vetorBinario=dec_to_vet(binario,END);
            aux.binario=vetorBinario;

            /*for(int i=0;i<END;i++){
                cout << aux.binario[i];
                if(i==(t-1) || i==(t+ind-1)) cout << " ";

            }*/
            //cout << endl;
            aux.tag=bin_to_dec(vetor_to_int(vetorBinario,0,t-1));
            aux.conjunto=bin_to_dec(vetor_to_int(vetorBinario,t,t+ind-1));
            aux.byte=bin_to_dec(vetor_to_int(vetorBinario,t+ind,t+ind+offs-1));
            //cout << "Tag: "<< aux.tag << " Conjunto: "<<aux.conjunto<<" Byte: "<<aux.byte<<endl;
            ///                   ///

            if (procuraCache(aux)){
              hitAC=1;
            }else{
                hitAC=0;
                gravaCache(aux,1);
            }

            break;
        }
    }
}
/// Funções estéticas //////////////////////////////////////////////////////

void printaMemoria(){
    int cor=0;
    for(int i=0;i<(tamMP/2);i++){
        if((i/tamBloco)%2==0) cor =1;
            else cor =0;

        textcolor(WHITE); textcolor(LIGHT_GREEN); printf("%.6d",dec_to_bin(i));textcolor(WHITE); printf("-->| ");
        if(cor==0) textcolor(YELLOW);if(cor==1) textcolor(LIGHT_GRAY); printf("%s ",mp[i].conteudo); textcolor(WHITE); printf("|");
        if(cor==0) textcolor(LIGHT_GRAY);if(cor==1) textcolor(YELLOW); printf(" %s",mp[i+32].conteudo);
        textcolor(WHITE);  printf("| <--"); textcolor(LIGHT_GREEN); printf("%.6d \n",dec_to_bin(i+32)); textcolor(WHITE);
    }
    textcolor(WHITE);
    gotoxy(72,1);
    printf("Memoria Principal (MP)\n");
    gotoxy(50,3);
    printf ("A esquerda nos temos a representacao de uma memoria principal de 64 bytes\n");
    gotoxy(50,5);
    printf("A memoria esta organizada em %d blocos (blocos cinzas e amarelos) de %d bytes cada",tamMP/tamBloco,tamBloco);
    gotoxy(50,7);
    printf("Cada celula da MP possui um"); textcolor(LIGHT_GREEN); printf(" endereco"); textcolor(WHITE); printf(" de 6 bits");
    gotoxy(50,8);
    printf("e um conteudo armazenado nela\n");
    gotoxy(50,10);
    printf("Formato: endereco-->| conteudo | conteudo |<--endereco\n");
    gotoxy(50,12);
    printf("Nesse programa, voce fara o papel de um software em execucao,\n");
    gotoxy(50,13);
    printf("solicitando um endereco da Memoria Principal.\n");
    gotoxy(50,15);
    system("Pause");
}
void printaLinha(int L,int C){
    cout<<endl;
    for(int i=0;i<tamBloco;i++){
        cout<<Ccache[C].lcache[L].bloco[i]<<endl;
    }
}

void printaCache(int tipo, int* instru){


    if(tipo==0){
        int line= bin_to_dec(vetor_to_int(instru,tmd,tmd+indmd-1));
        cout<<"Memoria Cache mapeada diretamente:\n\n";
        for(int i=0;i<MCACHE;i++){
            if(i==line)
                textcolor(BLUE);
            if(cacheMD[i].tag!=-1){
                printf("Linha %d:",i);
                    if(i==line){
                        textcolor(YELLOW); printf(" (Tag %.2d) |",dec_to_bin(cacheMD[i].tag)); textcolor(WHITE);
                    }else{
                        textcolor(WHITE); printf(" (Tag %.2d) |",dec_to_bin(cacheMD[i].tag));
                    }
            }
            else{
                printf("Linha %d:",i);
                if(i==line){
                    textcolor(YELLOW); printf(" (NULL!)  |");textcolor(WHITE);
                }else{
                    textcolor(WHITE); printf(" (NULL!)  |");
                }
            }
            for(int j=0;j<tamBloco;j++){
                if( j== bin_to_dec(vetor_to_int(instru,tmd+indmd,tmd+indmd+offsmd-1)) && i==line ) { textcolor(CYAN);}
                    else if(i==line && hitMD==0){ textcolor(LIGHT_CYAN);}
                if(hitMD==-1) textcolor(WHITE);
                printf(" %s ",cacheMD[i].bloco[j]); textcolor(WHITE);
                printf("|");
            }
            textcolor(LIGHT_MAGENTA); cout << " Conjunto " << i; textcolor(WHITE);
            cout<<endl;
        }
    }

    if(tipo==1){

        int blue=0;
        cout<<"Memoria Cache totalmente associativa:\n\n";
        for(int i=0;i<MCACHE;i++){
            if(hitTA==-1 && instru[0]==-5)
                textcolor(BLUE);
            if(cacheMD[i].tag!=-1){
                if(line==i){
                    textcolor(BLUE);
                    blue=1;
                    printf("Linha %d:",i);
                    if(hitTA==1){textcolor(YELLOW);}
                    printf(" (Tag %.4d) |",dec_to_bin(cacheMD[i].tag));
                }else{
                printf("Linha %d:",i);
                    printf(" (Tag %.4d) |",dec_to_bin(cacheMD[i].tag));
                }
            }
            else{
                printf("Linha %d:",i);
                printf(" (NULL!)    |");
            }
            for(int j=0;j<tamBloco;j++){
                if( j== bin_to_dec(vetor_to_int(instru,MCACHE,END-1)) && i==line ) { textcolor(CYAN);}
                    else if(i==line && hitTA==0){ textcolor(LIGHT_CYAN);}
                if(j== bin_to_dec(vetor_to_int(instru,MCACHE,END-1)) && i==line && hitTA==1) { textcolor(CYAN);}
                    else if(i==line && hitTA==1){ textcolor(WHITE);}
                //if(hitTA==-1) textcolor(WHITE);
                printf(" %s ",cacheMD[i].bloco[j]); //textcolor(WHITE);
                printf("|");
            }
            //cout << " " << cacheMD[i].lfu;
            textcolor(LIGHT_MAGENTA); cout << " Conjunto 0"; textcolor(WHITE);
            cout<<endl;
            if(blue==1){
                textcolor(WHITE);
                blue=0;
            }
        }
        textcolor(WHITE);
    }

    if(tipo==2){
        cout<<"Memoria Cache associativa por conjunto:\n\n";

        int conj=bin_to_dec(vetor_to_int(instru,t,t+ind-1));
        int blue=0;

        for(int i=0;i<qtdConjuntos;i++){
            for(int j=0;j<qtdLinhas;j++){
                if(i==conj && (line==-1 || line==j)){
                textcolor(BLUE);
                blue=1;
            }else blue=0;
                printf("Linha %d:",j);
                if(Ccache[i].lcache[j].tag!=-1){
                    if(hitAC && i==conj && line == j) textcolor(YELLOW);
                    printf(" (Tag %.3d) |",dec_to_bin(Ccache[i].lcache[j].tag)); if(blue && i == conj) textcolor(BLUE); else textcolor(WHITE);
                }else{
                    printf(" (NULL!)   |");
                }

                for(int k=0;k<tamBloco;k++){
                    if(hitAC==0 && i == conj && j == line && k == bin_to_dec(vetor_to_int(instru,tmd+indmd,tmd+indmd+offsmd-1))){ textcolor(CYAN); }
                     else if(hitAC==0 && i == conj && j == line){textcolor(LIGHT_CYAN);}
                        else if(hitAC==1 && i == conj && j == line && k == bin_to_dec(vetor_to_int(instru,tmd+indmd,tmd+indmd+offsmd-1))){textcolor(CYAN);}
                            else if(hitAC==1 && i == conj && j == line){textcolor(WHITE);}
                    printf(" %s ",Ccache[i].lcache[j].bloco[k]);
                    printf("|");
                }
                textcolor(LIGHT_MAGENTA); printf(" Conjunto %d\n",i,line); textcolor(WHITE);
            }
        printf("\n");
        textcolor(WHITE);
        }
    }
}

void printaBinario(int tipo,int* instru){
    cout << "Endereco interpretado pela cache: ";
    if(tipo==0){
        for(int i=0;i<END;i++){
            cout << instru[i];
            if(i==(tmd-1) || i==(tmd+indmd-1)) cout << " ";
        }
    }

    if(tipo==1){
        for(int i=0;i<END;i++){
            cout << instru[i];
            if(i==(MCACHE-1)) cout << " ";
        }

    }

    if(tipo==2){
        for(int i=0;i<END;i++){
            cout<< instru[i];
            if(i==(t-1) || i==(t+ind-1)) cout << " ";
        }
    }
    cout <<"\n";
}

void printaInfo(int tipo, int* instru){
    if(tipo==0){
        textcolor(YELLOW); cout <<"Tag: ";
        for(int i=0;i<END;i++){
            cout << instru[i];
            if (i==(tmd-1)){textcolor(BLUE); cout << " Linha: ";}
            if (i==(tmd+indmd-1)){textcolor(CYAN); cout << " Offset: ";}
            if (i==END-1) textcolor(WHITE);
        }
    }

    if(tipo==1){
        textcolor(YELLOW); cout <<"Tag: ";
        for(int i=0;i<END;i++){
            cout << instru[i];
            if (i==(MCACHE-1)){textcolor(CYAN); cout << " Offset: ";}
            if (i==END-1) textcolor(WHITE);
        }
    }

    if(tipo==2){
        textcolor(YELLOW); cout <<"Tag: ";
        for(int i=0;i<END;i++){
            cout << instru[i];
            if (i==(t-1)){textcolor(BLUE); cout << " Conjunto: ";}
            if (i==(t+ind-1)){textcolor(CYAN); cout << " Offset: ";}
            if (i==END-1) textcolor(WHITE);
        }
    }
    cout <<"\n";

}

void printaTela(int instru,int tipo){
    int FakeInstru[]={9,9,9,9,9,9};
    int VeriInstru[]={-5,-5,-5,-5,-5,-5};
    if(tipo==0){
        int mdtag = TelaMD.tag, mdindice=TelaMD.indice, mdoffset=TelaMD.offset;

        printaCache(tipo,FakeInstru);
        cout<<"\n\n\n";
        cout<<"\n\n\n";
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");

        printaCache(tipo,dec_to_vet(instru,END));
        cout<<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        cout<<"\n\n\n";
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");


        printaCache(tipo,dec_to_vet(instru,END));
        cout<<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        textcolor(LIGHT_GRAY); printf("\nVerificando se a tag da linha %d equivale a '%.2d'...\n\n",bin_to_dec(vetor_to_int(dec_to_vet(instru,6),tmd,tmd+indmd-1)),
                dec_to_bin(bin_to_dec(vetor_to_int(dec_to_vet(instru,6),0,tmd-1)))); textcolor(WHITE);
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");

        ciclo(instru,tipo);

        printaCache(tipo,dec_to_vet(instru,END));
        cout <<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        if(hitMD==1){
                textcolor(GREEN); printf("Cache hit!\n"); textcolor(WHITE); printf("Tags equivalentes! O "); textcolor(CYAN); printf("dado requerido");
                    textcolor(WHITE); printf(" ja se encontra na Cache!");
        }
        else{
                textcolor(RED); printf("Cache miss!\n"); textcolor(WHITE); printf("Tags nao equivalentes! O "); textcolor(CYAN); printf("dado requerido");
                    textcolor(WHITE); printf(" foi copiado da MP para a Cache junto com seus "); textcolor(LIGHT_CYAN); printf("vizinhos."); textcolor(WHITE);
        }
    }

    if(tipo==1){

        printaCache(tipo,FakeInstru);
        cout<<"\n\n\n";
        cout<<"\n\n\n";
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");

        printaCache(tipo,(instru,FakeInstru));
        cout<<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        cout<<"\n\n\n";
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");


        printaCache(tipo,VeriInstru);
        cout<<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        textcolor(LIGHT_GRAY); printf("\nVerificando se alguma linha possui a tag '%.4d'...\n\n", dec_to_bin(bin_to_dec(vetor_to_int(dec_to_vet(instru,6),0,MCACHE-1)))); textcolor(WHITE);
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");

        ciclo(instru,tipo);

        printaCache(tipo,dec_to_vet(instru,END));
        cout <<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        if(hitTA==1){
                textcolor(GREEN); printf("Cache hit!\n"); textcolor(WHITE); printf("Tag encontrada! O "); textcolor(CYAN); printf("dado requerido");
                    textcolor(WHITE); printf(" ja se encontra na Cache!");
        }
        else{
                textcolor(RED); printf("Cache miss!\n"); textcolor(WHITE); printf("Tag nao encontrada! O "); textcolor(CYAN); printf("dado requerido");
                    textcolor(WHITE); printf(" foi copiado da MP para a Cache junto com seus "); textcolor(LIGHT_CYAN); printf("vizinhos."); textcolor(WHITE);
        }
    }
    if(tipo==2){
        printaCache(tipo,FakeInstru);
        cout<<"\n\n\n";
        cout<<"\n\n\n";
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");

        printaCache(tipo,dec_to_vet(instru,END));
        cout<<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        cout<<"\n\n\n";
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");

        printaCache(tipo,dec_to_vet(instru,END));
        cout<<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        textcolor(LIGHT_GRAY); printf("\nVerificando se alguma linha do conjunto %d possui a Tag equivalente a '%.2d'...\n\n",bin_to_dec(vetor_to_int(dec_to_vet(instru,6),t,t+ind-1)),
                dec_to_bin(bin_to_dec(vetor_to_int(dec_to_vet(instru,6),0,t-1)))); textcolor(WHITE);
        printf("\n%.6d --> %s\n",instru,mp[bin_to_dec(instru)].conteudo);
        cout <<"\n";
        system("PAUSE");
        system("cls");


        ciclo(instru,tipo);

        printaCache(tipo,dec_to_vet(instru,END));
        cout <<"\n";
        printaBinario(tipo,dec_to_vet(instru,END));
        printaInfo(tipo,dec_to_vet(instru,END));
        if(hitAC==1){
                textcolor(GREEN); printf("Cache hit!\n"); textcolor(WHITE); printf("Tags equivalentes! O "); textcolor(CYAN); printf("dado requerido");
                    textcolor(WHITE); printf(" ja se encontra na Cache!");
        }
        else{
                textcolor(RED); printf("Cache miss!\n"); textcolor(WHITE); printf("Nao ha Tag equivalente! O "); textcolor(CYAN); printf("dado requerido");
                    textcolor(WHITE); printf(" foi copiado da MP para a Cache junto com seus "); textcolor(LIGHT_CYAN); printf("vizinhos."); textcolor(WHITE);
        }
    }
}

void menu(){
    do{
        system("cls");

        char op;
        int tipo;
        int instru;
        int exe[]={2,2,2,2,2,2};
        Restore();
        cout << "Vamos comecar!\n\nEntre com o modelo da memoria cache:\n\n1-Mapeada Diretamente\n2-Totalmente Associativa\n3-Associativa por conjunto\n\nou\n\n4-Ver representacao da MP\n\n";
        scanf("%d",&tipo);
        tipo-=1;
        if(tipo==3){
            system("cls"); printaMemoria();
        }
        else if(tipo>=0 && tipo <=2){
            system("cls");
            //cout<<"Pressione 'r' para ler uma instrucao (7 bits)\n 'l'+'valor da linha'+'valor do conjunto' para ler a linha de um conjunto ou 'e' para sair"<<endl;

            printaCache(tipo,exe);
            cout<<"\n\n\n\n";

            do{
                //cin>>op;
                line=-1;
                hitMD=-1;
                hitTA=-1;
                hitAC=-1;
                cout << "\n\nEntre com um numero binario de 6 bits ou digite '-1' para voltar\n";
                cin>>instru;
                system("cls");
                switch(instru){
                    case -1:{
                        //int instru;
                        //cin >> instru;
                        break;
                    }
                    default:{
                        //ciclo(instru,tipo);
                        printaTela(instru,tipo);
                        break;
                    }
                }
            }while(instru!=-1);
        }
    }while(1);
}

/// ///////////////////////////////////////////////////////////////////////////

int main(){
    system("mode con:cols=133 lines=33");
    GeraArquivo();
    IniciaMemoria(); //carrega arquivo para MP
    cout<<endl<<"Feito por: Andre Gomes - andregr@id.uff.br"<<endl<<"Projeto de Monitoria"<<endl<<"Fundamentos de Arquitetura de Computadores"<<endl<<"Universidade Federal Fluminense"<<endl<<"2018"<<endl;
    cout<<endl<<"\n\n\nAviso: Pode nao funcionar muito bem no Linux \n\n"<<endl;
    system("pause");
    system("cls");
    printaMemoria();
    menu();
    //int a[]={1,2,3,4,5,6};
    //printaCache(1,a);
}
