#pragma once //evita duplas inclusões

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef AUXILIARES_H
#define AUXILIARES_H

int termina_com_vc(const char *nome);

void deslocar_esq(const char *nome_arquivo_vina, long offset, long desloc, long tam_buffer);

void deslocar_dir(const char *nome_arquivo_vina, long offset, long desloc, long tam_buffer);

struct diretorio_t *carregar_diretorio(const char *nome_arquivo_vina);

struct diretorio_t *criar_diretorio();

struct membro_t *novo_info_desl_esq (struct diretorio_t *diretorio, int id_target, int id_membro);

struct membro_t *novo_info_desl_dir (struct diretorio_t *diretorio, int id_target, int id_membro);

int inserir_repetido (struct diretorio_t *diretorio, const char *nome_arquivo_vina, const char *nome_arquivo, int id_membro, const char *tipo);

int atualizar_diretorio_novo_membro(struct diretorio_t *diretorio, const char *nome_arquivo, long tam_entrada, long tam_comprimido);


#endif