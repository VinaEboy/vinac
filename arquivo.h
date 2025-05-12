#pragma once //evita duplas inclusões

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef ARQUIVO_H
#define ARQUIVO_H

#define MAX_CHAR_NOME 1024

struct diretorio_t {
    struct membro_t *info_membros;
    int total_membros;
    long int tam_maior_membro; //para caso precise de um buffer não pode ser maior que isso
};

struct membro_t { 
    char nome[MAX_CHAR_NOME];
    int UID;
    long int tamanho_original;
    long int tamanho_disco; //se for comprimido, esse é menor que o original
    time_t data_modificacao;
    int ordem; //ordem de inserção, pode mudar
    size_t localizacao; 
};

int inserir_p (const char *nome_arquivo_vina, const char *nome_arquivo);

int inserir_i (const char *nome_arquivo_vina, const char *nome_arquivo);

int remover(const char*nome_arquivo_vina, const char *membro_remover);

int extrair_arquivo(const char *nome_arquivo_vina, const char *nome_arquivo);

int mover(const char *nome_arquivo_vina, const char *nome_membro, const char *nome_target ) ;

int listar_diretorio(const char *nome_arquivo_vina);

#endif