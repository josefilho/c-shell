# Terminal Process Viewer 🖥️🔍

Este projeto é uma ferramenta de linha de comando desenvolvida em C que simula funcionalidades do terminal, incluindo:

- Listagem de diretórios (`ls -l`)
- Visualização de árvore de processos
- Conversão de tamanhos de arquivos para formato legível
- Verificação de permissões de arquivos
- Extração de informações de processos a partir do `/proc`
- Barra de progresso estética durante a compilação

## 🚀 Compilação

Compile o projeto utilizando o `Makefile` incluído:

```bash
make
```

Durante a compilação, uma barra de progresso será exibida para melhor feedback visual.

## 📂 Estrutura

- `main.c` — Contém o programa principal e implementação das funcionalidades.
- `term_tools.h` — Declarações das funções utilitárias.
- `Makefile` — Script de compilação com barra de progresso.
- `README.md` — Este arquivo.

## 🔧 Funcionalidades principais

### 1. `print_ls_details`
Simula a saída do `ls -l`, incluindo permissões, dono, grupo, tamanho, data e nome do arquivo.

### 2. `get_process_info`
Coleta informações detalhadas de um processo a partir de seu PID.

### 3. `print_process_tree`
Exibe recursivamente a árvore de processos, estilo `pstree`.

### 4. `mode_to_str` e `strmode`
Convertem o modo de arquivo (bits) para uma string como `-rwxr-xr--`.

### 5. `human_readable_size`
Converte bytes em formatos como `1.2K`, `3.4M`, etc.

## ⚙️ Requisitos

- Compilador C (GCC)
- Ambiente Unix/Linux (usa `/proc` e permissões POSIX)

## 🧹 Limpeza

Para remover os arquivos compilados:

```bash
make clean
```

## 📄 Licença

Este projeto é open-source e licenciado sob a [MIT License](LICENSE).

## ✍️ Autor

Desenvolvido por **José C. Gregório** 🚀  
Contribuições e sugestões são bem-vindas!
