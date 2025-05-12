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
//---------------------------------FUNCOES PRINCIPAIS---------------------------------------------- 
//-------------------------------------------------------------------------------------------------


//insere arquivo sem compressão, retorna 0 em caso de sucesso e difernete de 0 em erro
int inserir_p (const char *nome_arquivo_vina, const char *nome_arquivo) {
    struct diretorio_t *diretorio;

    //a main evita esse caso
    if (!nome_arquivo) {
        printf("Nome de arquivo a ser inserido é NULL \n");
        return -1;
    }

    if (strlen(nome_arquivo) > MAX_CHAR_NOME - 1) {
        printf("Erro ao inserir: nome de arquivo muito grande (maior que 1024B) \n");
        return 1;
    }

    FILE *arquivo_vina;
    // Tenta abrir para leitura e escrita (não apaga se já existir)
    arquivo_vina = fopen(nome_arquivo_vina, "rb+");   
    if (!arquivo_vina) {
        
        arquivo_vina = fopen(nome_arquivo_vina, "wb+"); // cria o arquivo vazio
        if (!arquivo_vina) {
            printf("Erro ao criar arquivo vazio \n");
            return 2;
        }
        diretorio = criar_diretorio();
        if (!diretorio) {
            printf("Erro ao alocar memória para diretorio \n");
            return 2;
        }

    } 
    else {
        diretorio = carregar_diretorio(nome_arquivo_vina);
        if (!diretorio) {
            printf("Erro ao carregar diretorio já existente \n");
            return 2;
        }
    }

    int id_membro = -1;
    for (int i=0; i < diretorio->total_membros; i++) 
        if (strcmp(nome_arquivo,diretorio->info_membros[i].nome) == 0) 
            id_membro = i;
    

    //caso em que já existe um arquivo com esse nome
    if (id_membro != -1) {
        int status = inserir_repetido(diretorio, nome_arquivo_vina, nome_arquivo, id_membro, "p");
        free(diretorio->info_membros);
        free(diretorio);
        if (status != 0) //se aconteceu algum erro em inserir_repetido retorna um valor de erro
            return 3;
        return 0; 
    }

    // Obter o tamanho do arquivo que vamos adicionar
    FILE *arquivo_adicionar = fopen(nome_arquivo, "rb"); //apenas a leitura é necessária
    if (!arquivo_adicionar) {
        printf("Erro ao abrir o arquivo para adicionar \n");
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 4;
    }
    fseek(arquivo_adicionar, 0, SEEK_END);
    long tamanho_arquivo_add = ftell(arquivo_adicionar);
    fseek(arquivo_adicionar, 0, SEEK_SET); //após obter o tamanho volta para o inicio


    // +1 porque vai inserir esse então tem que realocar
    //realloc não zera o novo espaço alocado
    struct membro_t *novo = realloc(diretorio->info_membros, 
        (diretorio->total_membros + 1) * sizeof(struct membro_t));

    if (!novo) {
        printf("Erro ao realocar info_membros \n");
        fclose(arquivo_vina);
        fclose(arquivo_adicionar);
        free(diretorio->info_membros);
        free(diretorio);
        return 5;
    } else {
        //zera o novo espaço alocado
        memset(&novo[diretorio->total_membros], 0, sizeof(struct membro_t));
        diretorio->info_membros = novo;
    }    

    if (atualizar_diretorio_novo_membro(diretorio,nome_arquivo, tamanho_arquivo_add, tamanho_arquivo_add) != 0) {
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 6; 
    }

    // Cria buffer e lê o conteúdo do arquivo a ser inserido
    char *buffer = malloc(tamanho_arquivo_add);
    if (!buffer) {
        printf("Erro ao alocar memória para o buffer \n");
        fclose(arquivo_adicionar);
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 7;
    }
    fread(buffer, 1, tamanho_arquivo_add, arquivo_adicionar);


    //CASO 1: foi inserido o primeiro arquivo no arquivo_vina (não existia antes)
    if (diretorio->total_membros == 1) {
        fwrite(diretorio, sizeof(struct diretorio_t), 1, arquivo_vina);
        fwrite(diretorio->info_membros, sizeof(struct membro_t), 1, arquivo_vina);
    } 
    else 
    {
        //desloca para direita a fim de criar um espaço "lixo" que vai 
        // ser sobrescrito com o novo struct membro_t
        //como a localizacao do membro 0 foi atualizada, tem que fazer em relação a localização antiga 
        deslocar_dir(nome_arquivo_vina, diretorio->info_membros[0].localizacao - sizeof(struct membro_t), 
        sizeof(struct membro_t), diretorio->tam_maior_membro );
        
        //sobrescreve o diretorio 
        fwrite(diretorio, sizeof(struct diretorio_t), 1, arquivo_vina);
        for (int i=0; i<diretorio->total_membros;i++) 
            fwrite(&diretorio->info_membros[i],sizeof(struct membro_t), 1, arquivo_vina);
    
    }
    fseek(arquivo_vina,0,SEEK_END);
    fwrite(buffer, tamanho_arquivo_add, 1, arquivo_vina); //escreve no final do arquivo

    fclose(arquivo_adicionar);
    fclose(arquivo_vina);
    free(buffer);
    free(diretorio->info_membros);
    free(diretorio);
    return 0;
}


//insere arquivo com compressão, retorna 0 em caso de sucesso 
int inserir_i (const char *nome_arquivo_vina, const char *nome_arquivo) {
    struct diretorio_t *diretorio;

    //a main evita esse caso
    if (!nome_arquivo) {
        printf("Nome de arquivo a ser inserido é NULL \n");
        return -1;
    }

    if (strlen(nome_arquivo) > MAX_CHAR_NOME - 1) {
        printf("Erro ao inserir: nome de arquivo muito grande (maior que 1024B) \n");
        return 1;
    }

    FILE *arquivo_vina;
    // Tenta abrir para leitura e escrita (não apaga se já existir)
    arquivo_vina = fopen(nome_arquivo_vina, "rb+"); 
    if (!arquivo_vina) {
        
        arquivo_vina = fopen(nome_arquivo_vina, "wb+"); // cria o arquivo vazio
        if (!arquivo_vina) {
            printf("Erro ao criar arquivo vazio \n");
            return 2;
        }
        diretorio = criar_diretorio();
        if (!diretorio) {
            printf("Erro ao alocar memória para diretorio \n");
            fclose(arquivo_vina);
            return 2;
        }

    } 
    else {
        diretorio = carregar_diretorio(nome_arquivo_vina);
        if (!diretorio) {
            printf("Erro ao carregar diretorio já existente \n");
            fclose(arquivo_vina);
            return 2;
        }
    }

    int id_membro = -1;
    for (int i=0; i < diretorio->total_membros; i++) 
        if (strcmp(nome_arquivo,diretorio->info_membros[i].nome) == 0) 
            id_membro = i;
    

    //Caso em que já existe um arquivo com esse nome
    if (id_membro != -1) {
        int status = inserir_repetido(diretorio, nome_arquivo_vina, nome_arquivo, id_membro, "i");
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        if (status != 0) //se aconteceu algum erro em inserir_repetido retorna um valor de erro
            return 3;
        return 0; 
    }


    FILE *arquivo_adicionar = fopen(nome_arquivo, "rb"); //só a leitura é necessária
    if (!arquivo_adicionar) {
        printf("Erro ao abrir o arquivo a ser inserido \n");
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 3;
    }

    // Obter tamanho do arquivo de entrada
    fseek(arquivo_adicionar, 0, SEEK_END);
    long tam_entrada = ftell(arquivo_adicionar);
    fseek(arquivo_adicionar, 0, SEEK_SET);

    // Alocar buffer para ler dados do arquivo original
    // aloca +3 posições para margem de segurança (evitar bufferoverflow);
    unsigned char *buffer_entrada = malloc(tam_entrada+3);
    if (!buffer_entrada) {
        printf("Erro ao alocar memória para o arquivo de entrada \n");
        fclose(arquivo_adicionar);
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 4;
    }

    fread(buffer_entrada, 1, tam_entrada, arquivo_adicionar);
    fclose(arquivo_adicionar);

    //um pouco mais de alocação para não dar estouro caso 
    //o arquivo tenha ficado maior comprimido
    unsigned char *dados_comprimidos = malloc(tam_entrada*1.3 + 64); //é truncado para size_t
    long tam_comprimido = LZ_Compress(buffer_entrada, dados_comprimidos, tam_entrada); 
    free(buffer_entrada);

    if (tam_comprimido < 0) {
        printf("Erro na compressão \n");
        fclose(arquivo_vina);
        free(dados_comprimidos);
        free(diretorio->info_membros);
        free(diretorio);
        return 5;
    }

    if (tam_comprimido >= tam_entrada) {
        printf("Arquivo %s será inserido sem compressão \n", nome_arquivo);
        fclose(arquivo_vina);
        //significa que criou um arquivo ".vc" vazio que iria ser preenchido por essa chamada
        if (diretorio->total_membros == 0) 
            remove(nome_arquivo_vina);
        int status = inserir_p(nome_arquivo_vina,nome_arquivo); //insere sem compressão

        free(dados_comprimidos);
        free(diretorio->info_membros);
        free(diretorio);
        return status;
    }
    printf("Arquivo %s será inserido comprimido \n", nome_arquivo);

    // Realloc para adicionar novo membro
    struct membro_t *novo = realloc(diretorio->info_membros, 
        (diretorio->total_membros + 1) * sizeof(struct membro_t));

    if (!novo) {
        printf("Erro ao realocar info_membros \n");
        fclose(arquivo_vina);
        free(dados_comprimidos);
        free(diretorio->info_membros);
        free(diretorio);
        return 6;
    } else {
        diretorio->info_membros = novo;
    }

    if (atualizar_diretorio_novo_membro(diretorio,nome_arquivo, tam_entrada, tam_comprimido) != 0) {
        fclose(arquivo_vina);
        free(dados_comprimidos);
        free(diretorio->info_membros);
        free(diretorio);
        return 7; 
    }

    // CASO 1: primeiro membro
    if (diretorio->total_membros == 1) {
        fwrite(diretorio, sizeof(struct diretorio_t), 1, arquivo_vina);
        fwrite(diretorio->info_membros, sizeof(struct membro_t), 1, arquivo_vina);
    } 
    else 
    {
        //desloca para direita a fim de criar um espaço "lixo" que vai 
        // ser sobrescrito com o novo struct membro_t
        //como a localizacao do membro 0 foi atualizada, tem que fazer em relação a localização antiga 
        deslocar_dir(nome_arquivo_vina, diretorio->info_membros[0].localizacao - sizeof(struct membro_t),sizeof(struct membro_t), diretorio->tam_maior_membro);

        fwrite(diretorio, sizeof(struct diretorio_t), 1, arquivo_vina);
        for (int i = 0; i < diretorio->total_membros; i++) {
            fwrite(&diretorio->info_membros[i], sizeof(struct membro_t), 1, arquivo_vina);
        }
    }
    //escreve no final o dado comprimido
    fseek(arquivo_vina, 0, SEEK_END);
    fwrite(dados_comprimidos, 1, tam_comprimido, arquivo_vina);

    free(dados_comprimidos);
    fclose(arquivo_vina);
    free(diretorio->info_membros);
    free(diretorio);

    return 0;
}


//extrai o arquivo (descompactado ou não) com aquele nome se ele existir, retorna 0 em caso de sucesso
int extrair_arquivo(const char *nome_arquivo_vina, const char *nome_arquivo) {

    //a main evita esse caso
    if (!nome_arquivo) {
        printf("Parâmetro NULL passado como nome de arquivo \n");
        return -1;
    }
    
    struct diretorio_t *diretorio = carregar_diretorio(nome_arquivo_vina);
    if (!diretorio) {
        printf("Erro ao carregar diretorio já existente \n");
        return 1;
    }
    

    FILE *arquivo_vina = fopen(nome_arquivo_vina, "rb");
    if (!arquivo_vina) {
        printf("Erro ao abrir arquivo Vina para leitura \n");
        free(diretorio->info_membros);
        free(diretorio);
        return 2;
    }

    int id_membro = -1;
    for (int i=0; i < diretorio->total_membros; i++) 
        if (strcmp(nome_arquivo,diretorio->info_membros[i].nome) == 0) 
            id_membro = i;

    if (id_membro == -1) {
        printf("Não existe um arquivo chamado %s no arquivo %s \n",nome_arquivo,nome_arquivo_vina);
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 3;
    }
    

    FILE *arquivo_extraido = fopen(nome_arquivo, "wb");
    if (!arquivo_extraido) {
        printf("Erro ao criar arquivo extraído \n");
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 4;
    }

    struct membro_t *membro = &diretorio->info_membros[id_membro]; //para diminuir a notação
    long offset = membro->localizacao;
    long tamanho = membro->tamanho_original;

    fseek(arquivo_vina, offset, SEEK_SET);
    char *buffer = malloc(tamanho);
    if (!buffer) {
        printf("Erro ao alocar memória para buffer \n");
        fclose(arquivo_vina);
        fclose(arquivo_extraido);
        free(diretorio->info_membros);
        free(diretorio);
        return 5;
    }


    size_t bytes_lidos = fread(buffer, 1, tamanho, arquivo_vina);
    
    //significa que é um arquivo compactado
    if (membro->tamanho_original != membro->tamanho_disco) {
        unsigned char *arquivo_descompactado = malloc(membro->tamanho_original);
        LZ_Uncompress((unsigned char*)buffer, arquivo_descompactado, membro->tamanho_disco);
        fwrite(arquivo_descompactado, 1, membro->tamanho_original, arquivo_extraido);
        free(arquivo_descompactado);
    }
    else 
        fwrite(buffer, 1, bytes_lidos, arquivo_extraido);

    
    free(buffer);
    fclose(arquivo_vina);
    fclose(arquivo_extraido);
    free(diretorio->info_membros);
    free(diretorio);        
    return 0;

}

//move o arquivo membro para depois de um target ou para o inicio se target null, retorna 0 em caso de sucesso
int mover(const char *nome_arquivo_vina, const char *nome_membro, const char *nome_target ) {
    struct diretorio_t *diretorio = carregar_diretorio(nome_arquivo_vina);
    if (!diretorio) {
        printf("Erro ao carregar diretorio já existente \n");
        return 1;
    }

    //a main evita esse caso
    if (diretorio->total_membros == 0) 
        return -1;

    // a main não permite nome_membro NULL
    if (!nome_membro) {
        printf("Nome de membro NULL \n");
        free(diretorio->info_membros);
        free(diretorio);
        return 2;
    }

    FILE *arquivo_vina = fopen(nome_arquivo_vina, "rb+");  // Tenta abrir para leitura e escrita
    if (!arquivo_vina) {
        printf(" Operação inválida para arquivo vazia: mover \n");
        free(diretorio->info_membros);
        free(diretorio);
        return 3;
    }

    int id_membro = -1;
    int id_target = -1; 
    for (int i=0; i < diretorio->total_membros; i++) {
        if (strcmp(nome_membro,diretorio->info_membros[i].nome) == 0) 
            id_membro = i;
        if (nome_target && strcmp(nome_target,diretorio->info_membros[i].nome) == 0)
            id_target = i;
        if ( id_membro != -1 && id_target != -1) break;
    }

    //não existe o arquivo de pelo menos 1 dos nomes passados
    if (id_membro == -1 || (nome_target && (id_target == -1) ) ) {
        if (id_membro == -1)
            printf("Não existe arquivo com o nome %s em %s \n",nome_membro,nome_arquivo_vina);
        if (nome_target && id_target == -1)
            printf("Não existe arquivo com o nome %s em %s \n",nome_target,nome_arquivo_vina);
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);        
        return 4;
    }

    long destino;
    //Caso em que target é NULL, vai mover para o começo
    if (id_target == -1) {
        destino = diretorio->info_membros[0].localizacao;
    } else {
        //se o target for o último, o destino é no final do arquivo
        if (id_target == diretorio->total_membros - 1) {
            destino = diretorio->info_membros[id_target].localizacao + diretorio->info_membros[id_target].tamanho_disco;
        } 
        else
        {
            destino = diretorio->info_membros[id_target+1].localizacao;
        }

    }

    //nada precisa ser feito se for mover imediatamente para frente de si mesmo
    //ou mover imediatamente atrás de si mesmo
    if (id_target == id_membro || (id_target == id_membro -1)) {
        fclose(arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 0; 
    }

    long tam_membro = diretorio->info_membros[id_membro].tamanho_disco;
    char *buffer = malloc(tam_membro);
    if (!buffer) {
        fclose(arquivo_vina);
        fprintf(stderr, "Erro ao alocar memória\n");
        return 5;
    }
    fseek(arquivo_vina,diretorio->info_membros[id_membro].localizacao,SEEK_SET); //coloca o ponteiro no membro que vou mover
    fread(buffer, 1, tam_membro, arquivo_vina); //membro a ser movido está no buffer


    //README tem ilustrações explicando essas etapas:
    //CASO 1: target à esquerda
    if (id_target < id_membro ) {
        //Etapa 1: alterar os binários dos arquivos
        deslocar_dir(nome_arquivo_vina, destino, tam_membro, tam_membro);
        fseek(arquivo_vina,destino,SEEK_SET);
        fwrite(buffer,1,tam_membro,arquivo_vina);
        deslocar_esq(nome_arquivo_vina, diretorio->info_membros[id_membro].localizacao + tam_membro, tam_membro, tam_membro);

        //Etapa 2: atualizar diretório
        struct membro_t *novo_info_membros = novo_info_desl_esq(diretorio,id_target,id_membro);
        diretorio->info_membros = novo_info_membros;
    } 
    else //CASO 2 : target à direita 
    {
        //Etapa 1: alterar os binários dos arquivos
        deslocar_esq(nome_arquivo_vina, diretorio->info_membros[id_membro].localizacao, tam_membro, diretorio->tam_maior_membro);
        deslocar_dir(nome_arquivo_vina, destino - tam_membro, tam_membro, tam_membro);
        fseek(arquivo_vina,destino-tam_membro,SEEK_SET);
        fwrite(buffer,1,tam_membro,arquivo_vina);

        //Etapa 2: atualizar diretório
        struct membro_t *novo_info_membros = novo_info_desl_dir(diretorio,id_target,id_membro);
        diretorio->info_membros = novo_info_membros;
    }

    //sobrescreve o diretorio 
    fseek(arquivo_vina,0,SEEK_SET);
    fwrite(diretorio, sizeof(struct diretorio_t), 1, arquivo_vina); 
    for (int i=0; i< diretorio->total_membros; i++) 
        fwrite(&diretorio->info_membros[i],sizeof(struct membro_t), 1, arquivo_vina);


    fclose(arquivo_vina);
    free(buffer);
    free(diretorio->info_membros);
    free(diretorio);
    return 0;
}

//lista as informações dos membros contidos no diretorio, retorna 0 em caso de sucesso
int listar_diretorio(const char *nome_arquivo_vina) {

    //a main evita esse caso
    if (!nome_arquivo_vina){
        printf("Nome de arquivo a ser listado é NULL \n");
        return -1;
    }

    struct diretorio_t *diretorio = carregar_diretorio(nome_arquivo_vina);
    if (!diretorio) {
        printf("Erro ao carregar diretorio já existente \n");
        return 1;
    }

    printf("Nome do arquivo vina: %s \n", nome_arquivo_vina);
    printf("Total de membros: %d\n\n", diretorio->total_membros);

    for (int i = 0; i < diretorio->total_membros; i++) {
        struct membro_t *membro = &diretorio->info_membros[i];

        printf("Nome do arquivo: %s\n", membro->nome); //tenho certeza que o nome não é nulo
        printf("UID: %d\n", membro->UID);
        printf("Tamanho original: %ld bytes\n", membro->tamanho_original);
        printf("Tamanho no disco: %ld bytes\n", membro->tamanho_disco);

        // Converte a data
        char *data_str = ctime(&membro->data_modificacao); //converte o tipo t_time para uma string legível
        if (!data_str) 
            printf("Data de modificação: Não definida (NULL) \n");
        else {
            data_str[strcspn(data_str, "\n")] = '\0';  // Remove '\n' da string
            printf("Data de modificação: %s\n", data_str);
        }

        printf("Ordem de inserção: %d\n", membro->ordem);
        printf("Localização no arquivo: %zu\n", membro->localizacao);
        
        if (i != diretorio->total_membros - 1)
            printf("\n");

    }

    free(diretorio->info_membros);
    free(diretorio);
    return 0;
}

//remove a informação do membro no diretório e seu arquivo binário do arquivo vina, retorna 0 em caso de sucesso
int remover(const char*nome_arquivo_vina, const char *membro_remover) {
    
    //a main evita esse caso
    if (!membro_remover) {
        printf("Nome de membro NULL \n");
        return -1;
    }

    struct diretorio_t *diretorio = carregar_diretorio(nome_arquivo_vina);
    if (!diretorio) {
        printf("Erro ao carregar diretorio no remover \n");
        return 1;
    }    

    int id_remover = -1;
    for (int i=0; i < diretorio->total_membros; i++) 
        if (strcmp(membro_remover,diretorio->info_membros[i].nome) == 0) 
            id_remover = i;
    
    if (id_remover == -1) {
        printf("Não existe um arquivo chamado %s no arquivo %s \n",membro_remover,nome_arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 2;
    }
    
    //movo o membro a ser removido para o final
    mover(nome_arquivo_vina,membro_remover,diretorio->info_membros[diretorio->total_membros-1].nome);
    free(diretorio->info_membros);
    free(diretorio);
    diretorio = carregar_diretorio(nome_arquivo_vina);
    if (!diretorio) {
        printf("Erro ao carregar diretorio atualizado depois de mover \n");
        return 3;
    }

    deslocar_esq(nome_arquivo_vina,diretorio->info_membros[0].localizacao-sizeof(struct membro_t),sizeof(struct membro_t), sizeof(struct membro_t));
    long tam_remover = diretorio->info_membros[id_remover].tamanho_disco;
    deslocar_esq(nome_arquivo_vina,diretorio->info_membros[diretorio->total_membros-1].localizacao,tam_remover,tam_remover);
    
    
    FILE *arquivo_vina = fopen(nome_arquivo_vina,"r+b"); //leitura e escrita
    if (!arquivo_vina) {
        printf("Erro ao abrir arquivo %s no remover \n",nome_arquivo_vina);
        free(diretorio->info_membros);
        free(diretorio);
        return 4;
    }

    diretorio->total_membros--;
    long maior = 0;
    for (int i=0; i<diretorio->total_membros; i++) {
        if (diretorio->info_membros[i].tamanho_disco > maior)
            maior = diretorio->info_membros[i].tamanho_disco;

        diretorio->info_membros[i].localizacao -= sizeof(struct membro_t);
    }

    
    fwrite(diretorio, 1, sizeof(struct diretorio_t), arquivo_vina);
    for (int i=0; i< diretorio->total_membros; i++) 
        fwrite(&diretorio->info_membros[i],sizeof(struct membro_t), 1, arquivo_vina);   


    fseek(arquivo_vina,0,SEEK_END);
    long novo_tamanho = ftell(arquivo_vina) - sizeof(struct membro_t);
    fclose(arquivo_vina); 
    
    // Agora que fiz todos os deslocamentos, sei exatamente o tamanho final do arquivo: 
    // o tamanho atual menos uma struct membro_t, que foi removida.”
    if (truncate(nome_arquivo_vina, novo_tamanho) != 0) {
        printf("Erro ao truncar o arquivo \n");
        free(diretorio->info_membros);
        free(diretorio);
        return 4;
    }

    //se o arquivo ficou vazio, exclui ele
    if (diretorio->total_membros == 0) 
        if (remove(nome_arquivo_vina) != 0) //a função remove da biblioteca padrão
            printf("Não foi possível excluir o arquivo vazio\n");


    free(diretorio->info_membros);
    free(diretorio);
    return 0;   
}