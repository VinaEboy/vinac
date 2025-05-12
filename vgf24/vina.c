#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arquivo.h"
#include "lz.h"
#include "auxiliares.h"

int main (int argc, char *argv[]) {

    if (argc < 3) {
        printf(" Argumentos insuficientes, tente novamente \n");
        return 1;
    }

    char *opcao = argv[1];
    int tam_opcao = strlen(argv[1]);
    char *nome_vina = argv[2]; //nome_vina não é NULL 


    if (termina_com_vc(nome_vina) != 1) {
        printf("Argumento <archive> inválido: não termina com '.vc' \n");
        return -2;
    }

    //vai verificar se arquivo vina existe
    FILE *arquivo_vina = fopen(nome_vina, "rb");
    if (!arquivo_vina) {

        // Se fopen falhou, o arquivo não existe
        if (strcmp(opcao,"-p" ) == 0 || strcmp(opcao,"-ip" ) == 0 ||
            strcmp(opcao, "-i") == 0 || strcmp(opcao,"-ic" ) == 0  ) {
            printf("Criando arquivo com nome %s \n",nome_vina);

        } else {
            printf("Opção inválida para um arquivo inexistente \n");
            return -1;
        }
    } 
    else 
        fclose(arquivo_vina); //após a verificação não é mais necessário
    

    if (tam_opcao > 3) {
        printf("Opção inválida\n");
        return 7;
    } 
    else if (tam_opcao == 3) {
        if (strcmp(opcao,"-ip") == 0) {
            if (argc == 3) {
                printf("Nenhum nome de arquivo para ser adicionado foi passado \n");
                return 1;
            }
            for (int i = 3; i < argc; i++) 
                if (inserir_p(nome_vina,argv[i]) != 0) {
                    printf("Erro ao inserir membro número %d \n",i-2);
                    return 1;
                }
        }

        else if(strcmp(opcao,"-ic") == 0) {
            if (argc == 3) {
                printf("Nenhum nome de arquivo para ser adicionado foi passado \n");
                return 2;
            }
            for (int i=3; i<argc; i++) 
                if (inserir_i(nome_vina,argv[i])!= 0) {
                    printf("Erro ao inserir arquivo chamado %s \n",argv[i]);
                    return 2;
                }
        } 
    
        else {
            printf("Opção inválida\n");
            return 7;    
        }
        return 0;
    }


    switch (opcao[1]) { 

        case 'p': 
            if (argc == 3) {
                printf("Nenhum nome de arquivo para ser adicionado foi passado \n");
                return 1;
            }
            for (int i = 3; i < argc; i++) 
                if (inserir_p(nome_vina,argv[i]) != 0) {
                    printf("Erro ao inserir membro número %d \n",i-2);
                    return 1;
                }
            break;
        
        case 'i':
            if (argc == 3) {
                printf("Nenhum nome de arquivo para ser adicionado foi passado \n");
                return 2;
            }
            for (int i=3; i<argc; i++) 
                if (inserir_i(nome_vina,argv[i])!= 0) {
                    printf("Erro ao inserir arquivo chamado %s \n",argv[i]);
                    return 2;
                }
            
            break;

        case 'r':
            if (argc == 3) {
                printf("Nenhum arquivo para remover especificado \n");
                return 3;
            }
            for (int i=3; i < argc; i++) 
                if (remover(nome_vina,argv[i]) != 0) {
                    printf("Erro ao remover arquivo chamado %s \n", argv[i]);
                    return 3;
                }
            break;

        case 'c':
            if (listar_diretorio(nome_vina) != 0) {
                printf("Erro ao listar membros do arquivo \n");
                return 4;
            }
            break;

        case 'x':
            if (argc == 3) { //Todos serão extraidos
                struct diretorio_t *diretorio = carregar_diretorio(nome_vina);
                for (int i=0; i < diretorio->total_membros; i++) {
                    if(extrair_arquivo(nome_vina,diretorio->info_membros[i].nome) != 0) {
                        printf("Erro ao extrair arquivo chamado %s \n",diretorio->info_membros[i].nome);
                        return 5;
                    } 
                } 
                free(diretorio->info_membros);
                free(diretorio);           
            }
            for (int i=3; i<argc; i++)
                if(extrair_arquivo(nome_vina,argv[i]) != 0) {
                    printf("Erro ao extrair arquivo chamado %s \n",argv[i]);
                    return 5;
                }
            break;

        case 'm':
            if (argc < 4) {
                printf("Nenhum membro para mover especificado \n");
                return 6;
            }
            else if (argc == 4) { //target é NULL (não tem nome) então vai mover para o ínicio
                if (mover (nome_vina, argv[3], NULL) != 0) {
                    printf("Erro ao mover membro %s para o ínicio \n", argv[3]);
                    return 6;
                }
            }
            else if (argc == 5) {
                if (mover(nome_vina, argv[3], argv[4]) != 0) {
                    printf("Erro ao mover membro %s após o membro %s \n", argv[3],argv[4]);
                    return 6;
                }
            }
            else {
                printf("Muitos parâmetros para a operação -m \n");
                return 6;
            }
            break;

        default:
            printf("Opção inválida\n");
            return 7;
    }

    return 0;
}