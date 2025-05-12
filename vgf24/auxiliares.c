#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "arquivo.h"
#include "lz.h"
#include "auxiliares.h"

//-------------------------------------------------------------------------------------------------
//---------------------------------FUNCOES AUXILIARES---------------------------------------------- 
//-------------------------------------------------------------------------------------------------


int termina_com_vc(const char *nome) {
    size_t len = strlen(nome);

    // não pode terminar com ".vc" se for menor que 3 caracteres
    if (len < 3) 
        return 0; 

    //nome é um ponteiro para char, então estou comparado o ponteiro
    //a partir das últimas três posições
    return strcmp(nome + len - 3, ".vc") == 0;
}


// começa no início do arquivo. Ela respeita o parâmetro passado `offset`, ou seja, se for para deslocar para esquerda 
// um byte que sobrescreveria uma parte do binário que está antes do início, a função não sobrescreve essa parte e descarta esses bytes. 
// Ao final é feito um `truncate` para reduzir o tamanho do arquivo em "deslocamento".
void deslocar_esq(const char *nome_arquivo_vina, long offset, long desloc, long tam_buffer) {
    if (!nome_arquivo_vina || (offset < 0) || (tam_buffer < 0) || (desloc < 0)) {
        perror("Erro nos parâmetros de deslocar_esq");
        return;
    }

    FILE *arquivo_vina = fopen(nome_arquivo_vina, "r+b");
    if (!arquivo_vina) {
        perror("Erro ao abrir o arquivo para deslocar");
        return;
    }

    fseek(arquivo_vina, 0, SEEK_END);
    long tam_arquivo_vina = ftell(arquivo_vina);

    // Aloca um buffer para ler e escrever os dados remanescentes
    char *buffer = malloc(tam_buffer); 
    if (!buffer) {
        perror("Erro ao alocar buffer");
        return;
    }
    size_t lido;

    long origem = offset + desloc;
    long destino = offset;

    fseek(arquivo_vina,origem,SEEK_SET);
    //o lido me diz quantos bytes eu li, e se acabar vai ler 0
    while ((lido = fread(buffer, 1, tam_buffer, arquivo_vina)) > 0) {
        fseek(arquivo_vina, destino, SEEK_SET);
        fwrite(buffer, 1, lido, arquivo_vina);
        origem += lido;
        destino += lido;
        fseek(arquivo_vina, origem, SEEK_SET);
    }


    fflush(arquivo_vina);  // Garante que tudo foi escrito (o que tá no buffer é escrito com certeza)

    fclose(arquivo_vina);  
    if (offset <= tam_arquivo_vina - desloc) //trunca o final do arquivo 
        truncate(nome_arquivo_vina, tam_arquivo_vina - desloc);
    else   //respeita o offset, nada antes dele é truncado
        truncate(nome_arquivo_vina, offset); 

    free(buffer);
}


// começa pelo fim do arquivo e vai deslocando para a direita até chegar no parâmetro passado `offset`. 
// É feito um `truncate` para aumentar o tamanho do arquivo em "deslocamento".
// Gera lixo se deslocamento > tamanho_arquivo_vina - offset
void deslocar_dir(const char *nome_arquivo_vina, long offset, long desloc, long tam_buffer) {
    if (!nome_arquivo_vina || (offset < 0) || (tam_buffer < 0) || (desloc < 0)) {
        perror("Erro nos parâmetros de deslocar_dir");
        return;
    }

    FILE *arquivo_vina = fopen(nome_arquivo_vina, "r+b");
    if (!arquivo_vina) {
        perror("Erro ao abrir o arquivo para deslocar para a direita");
        return;
    }

    // Move até o final do arquivo para saber seu tamanho
    fseek(arquivo_vina, 0, SEEK_END);
    long tamanho_total = ftell(arquivo_vina);

    // O deslocamento só é necessário se houver dados após o offset
    if (offset >= tamanho_total) {
        fclose(arquivo_vina);
        return;
    }


    //Caso em que acessaria um índice negativo no arquivo porque não teria
    //conteúdo para deslocar para direita (emprestar de antes do offset)
    if (tamanho_total < desloc ) {
        perror("Não é possível deslocar para direita o conteúdo, seria necessário inserir 0's no deslocamento");
        fclose(arquivo_vina);
        return;
    }

    char *buffer = malloc(tam_buffer);
    if (!buffer) {
        perror("Erro ao alocar buffer");
        fclose(arquivo_vina);
        return;
    }

    long origem = tamanho_total;
    long destino = origem + desloc;


    fclose(arquivo_vina);
    truncate(nome_arquivo_vina, tamanho_total + desloc);
    arquivo_vina = fopen(nome_arquivo_vina,"r+b");
    fseek(arquivo_vina, 0, SEEK_END);

    // Faz o deslocamento de trás para frente, em blocos
    while (origem > offset) {
        long tamanho_bloco = tam_buffer;
        if (origem - tamanho_bloco < offset)
            tamanho_bloco = origem - offset;

        origem -= tamanho_bloco;
        destino -= tamanho_bloco;

        fseek(arquivo_vina, origem, SEEK_SET);
        fread(buffer, 1, tamanho_bloco, arquivo_vina);

        fseek(arquivo_vina, destino, SEEK_SET);
        fwrite(buffer, 1, tamanho_bloco, arquivo_vina);
    }

    free(buffer);
    fclose(arquivo_vina);
}

//interpreta os dados do arquivo vina em uma struct diretório
struct diretorio_t *carregar_diretorio(const char *nome_arquivo_vina) {
    FILE *arquivo_vina = fopen(nome_arquivo_vina, "rb");
    if (!arquivo_vina) {
        perror("Erro ao abrir o arquivo Vina ao carregar vina");
        return NULL;
    }

    struct diretorio_t *diretorio = malloc(sizeof(struct diretorio_t));
    if (!diretorio) {
        perror("Erro ao alocar memória para a struct diretorio_t");
        fclose(arquivo_vina);
        return NULL;
    }
    memset(diretorio, 0, sizeof(struct diretorio_t)); 

    // Lê o cabeçalho
    if (fread(diretorio, sizeof(struct diretorio_t), 1, arquivo_vina) != 1) {
        perror("Erro ao ler o diretório");
        free(diretorio);
        fclose(arquivo_vina);
        return NULL;
    }

    // Agora precisamos alocar e ler os membros
    if (diretorio->total_membros > 0) {
        diretorio->info_membros = malloc(sizeof(struct membro_t) * diretorio->total_membros);
        memset(diretorio->info_membros, 0,sizeof(struct membro_t)*diretorio->total_membros); 
        if (!diretorio->info_membros) {
            perror("Erro ao alocar memória para membros");
            free(diretorio);
            fclose(arquivo_vina);
            return NULL;
        }

        if (fread(diretorio->info_membros, sizeof(struct membro_t), diretorio->total_membros, arquivo_vina) != (size_t)diretorio->total_membros) {
            perror("Erro ao ler os membros do arquivo");
            free(diretorio->info_membros);
            free(diretorio);
            fclose(arquivo_vina);
            return NULL;
        }
    } else {
        diretorio->info_membros = NULL; // Não há membros
    }

    fclose(arquivo_vina);
    return diretorio;
}

//cria a struct diretorio para ser o cabeçalho do arquivo vina
struct diretorio_t *criar_diretorio() {
    struct diretorio_t *diretorio = malloc(sizeof(struct diretorio_t));
    if (!diretorio)
        return NULL;
    memset(diretorio, 0, sizeof(struct diretorio_t)); 

    diretorio->info_membros = NULL;
    diretorio->total_membros = 0;
    diretorio->tam_maior_membro = 0; //para usar na alocação do bufferWW

    return diretorio;
}

//atualiza o diretório após a função move no CASO 1
struct membro_t *novo_info_desl_esq (struct diretorio_t *diretorio, int id_target, int id_membro) {

    struct membro_t *novo_info_membro = malloc(diretorio->total_membros*sizeof(struct membro_t));
    memset(novo_info_membro, 0, diretorio->total_membros*sizeof(struct membro_t)); 

    long loc = sizeof(struct diretorio_t) + (diretorio->total_membros*sizeof(struct membro_t));
    for (int i=0; i <= id_target; i++) {
            strcpy(novo_info_membro[i].nome,diretorio->info_membros[i].nome); 
            novo_info_membro[i].UID = diretorio->info_membros[i].UID;
            novo_info_membro[i].tamanho_original = diretorio->info_membros[i].tamanho_original; 
            novo_info_membro[i].tamanho_disco = diretorio->info_membros[i].tamanho_disco;
            novo_info_membro[i].data_modificacao = diretorio->info_membros[i].data_modificacao;
            novo_info_membro[i].ordem = i;
            novo_info_membro[i].localizacao = loc;
            loc += novo_info_membro[i].tamanho_disco;
    }

    strcpy(novo_info_membro[id_target+1].nome,diretorio->info_membros[id_membro].nome); 
    novo_info_membro[id_target+1].UID = diretorio->info_membros[id_membro].UID;
    novo_info_membro[id_target+1].tamanho_original = diretorio->info_membros[id_membro].tamanho_original; 
    novo_info_membro[id_target+1].tamanho_disco = diretorio->info_membros[id_membro].tamanho_disco;
    novo_info_membro[id_target+1].data_modificacao = diretorio->info_membros[id_membro].data_modificacao;
    novo_info_membro[id_target+1].ordem = id_target+1;
    novo_info_membro[id_target+1].localizacao = loc;
    loc += novo_info_membro[id_target+1].tamanho_disco;

    for (int i=id_target+2; i <= id_membro; i++) {
            strcpy(novo_info_membro[i].nome,diretorio->info_membros[i-1].nome); 
            novo_info_membro[i].UID = diretorio->info_membros[i-1].UID;
            novo_info_membro[i].tamanho_original = diretorio->info_membros[i-1].tamanho_original; 
            novo_info_membro[i].tamanho_disco = diretorio->info_membros[i-1].tamanho_disco;
            novo_info_membro[i].data_modificacao = diretorio->info_membros[i-1].data_modificacao;
            novo_info_membro[i].ordem = i;
            novo_info_membro[i].localizacao = loc;
            loc += novo_info_membro[i].tamanho_disco;
    }        

    for (int i=id_membro+1; i < diretorio->total_membros; i++) {
            strcpy(novo_info_membro[i].nome,diretorio->info_membros[i].nome); 
            novo_info_membro[i].UID = diretorio->info_membros[i].UID;
            novo_info_membro[i].tamanho_original = diretorio->info_membros[i].tamanho_original; 
            novo_info_membro[i].tamanho_disco = diretorio->info_membros[i].tamanho_disco;
            novo_info_membro[i].data_modificacao = diretorio->info_membros[i].data_modificacao;
            novo_info_membro[i].ordem = i;
            novo_info_membro[i].localizacao = loc;
            loc += novo_info_membro[i].tamanho_disco;
    }   

    free(diretorio->info_membros);
    return novo_info_membro;
}

//atualiza o diretório após a função move no CASO 2
struct membro_t *novo_info_desl_dir (struct diretorio_t *diretorio, int id_target, int id_membro) {

    struct membro_t *novo_info_membro = malloc(diretorio->total_membros*sizeof(struct membro_t));
    memset(novo_info_membro, 0, diretorio->total_membros*sizeof(struct membro_t)); 

    long loc = sizeof(struct diretorio_t) + (diretorio->total_membros*sizeof(struct membro_t));
    for (int i=0; i < id_membro; i++) {
            strcpy(novo_info_membro[i].nome,diretorio->info_membros[i].nome); 
            novo_info_membro[i].UID = diretorio->info_membros[i].UID;
            novo_info_membro[i].tamanho_original = diretorio->info_membros[i].tamanho_original; 
            novo_info_membro[i].tamanho_disco = diretorio->info_membros[i].tamanho_disco;
            novo_info_membro[i].data_modificacao = diretorio->info_membros[i].data_modificacao;
            novo_info_membro[i].ordem = i;
            novo_info_membro[i].localizacao = loc;
            loc += novo_info_membro[i].tamanho_disco;
    }

    for (int i=id_membro; i < id_target; i++) {
            strcpy(novo_info_membro[i].nome,diretorio->info_membros[i+1].nome); 
            novo_info_membro[i].UID = diretorio->info_membros[i+1].UID;
            novo_info_membro[i].tamanho_original = diretorio->info_membros[i+1].tamanho_original;
            novo_info_membro[i].tamanho_disco = diretorio->info_membros[i+1].tamanho_disco;
            novo_info_membro[i].data_modificacao = diretorio->info_membros[i+1].data_modificacao;
            novo_info_membro[i].ordem = i;
            novo_info_membro[i].localizacao = loc;
            loc += novo_info_membro[i].tamanho_disco;
    }    
    
    strcpy(novo_info_membro[id_target].nome,diretorio->info_membros[id_membro].nome); 
    novo_info_membro[id_target].UID = diretorio->info_membros[id_membro].UID;
    novo_info_membro[id_target].tamanho_original = diretorio->info_membros[id_membro].tamanho_original; 
    novo_info_membro[id_target].tamanho_disco = diretorio->info_membros[id_membro].tamanho_disco;
    novo_info_membro[id_target].data_modificacao = diretorio->info_membros[id_membro].data_modificacao;
    novo_info_membro[id_target].ordem = id_target;
    novo_info_membro[id_target].localizacao = loc;
    loc += novo_info_membro[id_target].tamanho_disco;

    for (int i=id_target+1; i < diretorio->total_membros; i++) {
            strcpy(novo_info_membro[i].nome,diretorio->info_membros[i].nome); 
            novo_info_membro[i].UID = diretorio->info_membros[i].UID;
            novo_info_membro[i].tamanho_original = diretorio->info_membros[i].tamanho_original; 
            novo_info_membro[i].tamanho_disco = diretorio->info_membros[i].tamanho_disco;
            novo_info_membro[i].data_modificacao = diretorio->info_membros[i].data_modificacao;
            novo_info_membro[i].ordem = i;
            novo_info_membro[i].localizacao = loc;
            loc += novo_info_membro[i].tamanho_disco;
    }   

    free(diretorio->info_membros);
    return novo_info_membro;
}

int inserir_repetido (struct diretorio_t *diretorio, const char *nome_arquivo_vina, const char *nome_arquivo, int id_membro, const char *tipo) {
    int status = 0;

    if (remover(nome_arquivo_vina, nome_arquivo) != 0) {
        printf("Erro ao remover arquivo repetido \n");
        status = 1;
    }

    if (strcmp(tipo, "i") == 0) {
        if (inserir_i(nome_arquivo_vina, nome_arquivo) != 0) {
            printf("Erro ao inserir arquivo substituído \n");
            status = 2;
        }
    } 
    else if (strcmp(tipo, "p") == 0)  {

        if (inserir_p(nome_arquivo_vina, nome_arquivo) != 0) {
            printf("Erro ao inserir arquivo substituído \n");
            status = 2;
        }
    }
    else {
        printf("Parâmetro inválido para o tipo de inserção: %s \n", tipo);
        status = 3;
    }

    if (id_membro == 0) {
        if (mover(nome_arquivo_vina, nome_arquivo, NULL) != 0) {
            printf("Erro ao mover arquivo substituído para o começo \n");
            status = 4;
        }
    } else {
        if (mover(nome_arquivo_vina, nome_arquivo, diretorio->info_membros[id_membro - 1].nome) != 0) {
            printf("Erro ao mover arquivo substituído para a posição \n");
            status = 5;
        }
    }

    return status;
}

int atualizar_diretorio_novo_membro(struct diretorio_t *diretorio, const char *nome_arquivo, long tam_original, long tam_disco) {

    struct stat info;
    if (stat(nome_arquivo, &info) != 0) {
        printf("Erro ao obter metadados\n");
        return 1;
    }

    int id = diretorio->total_membros;

    strcpy(diretorio->info_membros[id].nome, nome_arquivo);
    diretorio->info_membros[id].UID = info.st_uid;
    diretorio->info_membros[id].tamanho_original = tam_original;
    diretorio->info_membros[id].tamanho_disco = tam_disco;
    diretorio->info_membros[id].data_modificacao = info.st_mtime;
    diretorio->info_membros[id].ordem = id;

    // Atualiza localização
    if (diretorio->total_membros == 0) {
        diretorio->info_membros[0].localizacao = sizeof(struct diretorio_t) + sizeof(struct membro_t);
    } else {
        for (int i = 0; i < diretorio->total_membros; i++) {
            diretorio->info_membros[i].localizacao += sizeof(struct membro_t);
        }
        diretorio->info_membros[id].localizacao = diretorio->info_membros[id - 1].localizacao + diretorio->info_membros[id - 1].tamanho_disco;
    }

    //atualiza tam_maior_membro
    if (tam_disco > diretorio->tam_maior_membro)
        diretorio->tam_maior_membro = tam_disco;

    diretorio->total_membros++;

    return 0;
}