#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <libgen.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/dir.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "term_tools.h"

#define MAX_PROCESSES 1024

/*****************************************************************************/

struct process_info {
    pid_t pid;
    pid_t ppid;
    char name[256];
};

/*****************************************************************************/

char *_PATH;
size_t TERM_HEIGHT = 0;
size_t TERM_WIDTH = 0;

/*****************************************************************************/

/**
 * @brief Exibe uma mensagem de boas-vindas no terminal.
 */
void welcome_message();

/**
 * @brief Manipula o sinal SIGCHLD para evitar processos zumbis.
 * @param sig O número do sinal recebido (normalmente SIGCHLD).
 */
void sigchld_handler(int sig);

/**
 * @brief Obtém informações de um processo a partir do PID.
 * @param pid O PID do processo a ser analisado.
 * @return Estrutura `process_info` preenchida com dados do processo.
 */
struct process_info get_process_info(pid_t pid);

/**
 * @brief Exibe a árvore de processos recursivamente.
 * @param pid PID do processo inicial.
 * @param depth Nível atual de profundidade na árvore.
 * @param is_last Indica se este é o último processo do nível atual.
 * @param ancestors Vetor booleano para rastrear os níveis da árvore.
 */
void print_process_tree(pid_t pid, int depth, bool is_last, const bool *ancestors);

/**
 * @brief Verifica se uma string representa um número válido.
 * @param str A string a ser analisada.
 * @return true se for um número, false caso contrário.
 */
bool is_number(const char *str);

/**
 * @brief Exibe os detalhes de arquivos no estilo do comando `ls -l`.
 * @param path Caminho do diretório ou arquivo.
 * @param show_all Se true, também exibe arquivos ocultos.
 */
void print_lf_details(const char *path, bool show_all);

/**
 * @brief Função de comparação para ordenação de entradas de diretório.
 * @param a Ponteiro para o primeiro elemento.
 * @param b Ponteiro para o segundo elemento.
 * @return Valor < 0 se a < b, 0 se iguais, > 0 se a > b.
 */
int compare_entries(const void *a, const void *b);

/**
 * @brief Converte um modo de arquivo (bits de permissão) em string legível.
 * @param mode Bits do modo do arquivo (mode_t).
 * @param str String de saída onde será escrita a representação do modo.
 */
void strmode(mode_t mode, char *str);

/**
 * @brief Função de comparação para ordenação de PIDs.
 * @param a Ponteiro para o primeiro PID.
 * @param b Ponteiro para o segundo PID.
 * @return Valor < 0 se a < b, 0 se iguais, > 0 se a > b.
 */
int compare_pids(const void *a, const void *b);

/**
 * @brief Converte o modo de arquivo para string no formato `-rwxr-xr--`.
 * @param mode Modo do arquivo.
 * @param str Buffer onde a string será armazenada.
 */
static void mode_to_str(mode_t mode, char *str);

/**
 * @brief Converte um valor em bytes para uma string com tamanho legível.
 * @param bytes Valor em bytes.
 * @return String com o valor em formato como "1.2K", "3.4M", etc.
 */
static char *human_readable_size(long bytes);

/*****************************************************************************/

int main() {
    system("clear");
    _PATH = malloc(2048);
    // Configurar o tratamento de SIGCHLD para evitar processos zumbi
    signal(SIGCHLD, sigchld_handler);

    // Preparar o ambiente
    TERM_WIDTH = (int) getTerminalCols() > 0 ? (int) getTerminalCols() : 80;
    TERM_HEIGHT = (int) getTerminalRows() > 0 ? (int) getTerminalRows() : 24;
    struct passwd *pw = getpwuid(getuid());
    char username[256];
    strcpy(username, pw->pw_name);

    char cwd[2048];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("%sMAIN: Erro ao obter diretório atual %d\n", TERM_RED_BOLD, __LINE__);
        exit(1);
    }

    welcome_message();
    bool last_command_exit_error = false;

    while (true) {
        // Prompt do usuário
        printf("%s╭─%s%s%s in %s%s%s\n",
               TERM_CYAN, TERM_CYAN_BOLD, username, TERM_RESET,
               TERM_YELLOW_ITALIC, cwd, TERM_RESET);
        printf("%s╰───%s❭ %s",
               TERM_CYAN,
               last_command_exit_error ? TERM_RED_BOLD : TERM_GREEN_BOLD,
               TERM_RESET);

        char input[2048];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("%sMAIN: Erro ao ler entrada %d\n", TERM_RED_BOLD, __LINE__);
            last_command_exit_error = true;
            continue;
        }

        // Remover nova linha
        input[strcspn(input, "\n")] = '\0';

        // Comando vazio
        if (strlen(input) == 0) continue;

        // Dividir em argumentos
        char *args[1024];
        int arg_count = 0;
        char *token = strtok(input, " \t");
        while (token != NULL && arg_count < 1023) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t");
        }
        args[arg_count] = NULL;

        // Verificar se é background
        bool background = false;
        if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
            background = true;
            args[--arg_count] = NULL;
        }

        // Comandos internos
        if (arg_count == 0) continue;
        else if (strcmp(args[0], "exit") == 0) break;
        else if (strcmp(args[0], "help") == 0) {
            printf("\n%sComandos disponíveis:%s\n", TERM_CYAN_BOLD, TERM_RESET);
            printf("%sexit    %s- %sSair do shell%s\n", TERM_CYAN_BOLD, TERM_RESET, TERM_GREEN, TERM_RESET);
            printf("%shelp    %s- %sMostrar ajuda%s\n", TERM_CYAN_BOLD, TERM_RESET, TERM_GREEN, TERM_RESET);
            printf("%scd      %s- %sMudar diretório%s\n", TERM_CYAN_BOLD, TERM_RESET, TERM_GREEN, TERM_RESET);
            printf("%slf      %s- %sListar diretório%s\n", TERM_CYAN_BOLD, TERM_RESET, TERM_GREEN, TERM_RESET);
            printf("%stree    %s- %sÁrvore de processos%s\n", TERM_CYAN_BOLD, TERM_RESET, TERM_GREEN, TERM_RESET);
            printf("\n%sUse '%s&%s' no final para executar em segundo plano\n", TERM_WHITE, TERM_YELLOW_ITALIC,
                   TERM_RESET);
            last_command_exit_error = false;
        } else if (strcmp(args[0], "cd") == 0) {
            char *path = (arg_count > 1) ? args[1] : getenv("HOME");
            if (chdir(path) != 0) {
                printf("%sErro ao mudar para %s%s\n", TERM_RED_BOLD, path, TERM_RESET);
                last_command_exit_error = true;
            } else {
                if (!getcwd(cwd, sizeof(cwd))) {
                    printf("%sErro ao obter diretório%s\n", TERM_RED_BOLD, TERM_RESET);
                    last_command_exit_error = true;
                } else last_command_exit_error = false;
            }
        } else if (strcmp(args[0], "lf") == 0) {
            if (arg_count > 1 && (strcmp(args[1], "-h") == 0 || strcmp(args[1], "--help") == 0)) {
                // Ajuda
                printf("%sUso: lf [OPÇÕES] [DIRETÓRIO]%s\n", TERM_CYAN_BOLD, TERM_RESET);
                printf("Listar conteúdo do diretório\n\n");
                printf("%sOpções:%s\n", TERM_YELLOW_BOLD, TERM_RESET);
                printf("  -a\t\tMostrar arquivos ocultos\n");
                printf("  -l\t\tFormato detalhado\n");
                printf("  --help\t\tExibir esta ajuda\n");
                last_command_exit_error = false;
                continue;
            }

            bool long_format = false;
            bool show_all = false;
            const char *path = cwd;

            // Processar flags
            for (int i = 1; i < arg_count; i++) {
                if (strcmp(args[i], "-l") == 0) long_format = true;
                else if (strcmp(args[i], "-a") == 0) show_all = true;
                else if (strcmp(args[i], "-la") == 0 || strcmp(args[i], "-al") == 0) {
                    long_format = show_all = true;
                }
            }

            // Obter path se especificado
            for (int i = 1; i < arg_count; i++) {
                if (args[i][0] != '-') {
                    path = args[i];
                    break;
                }
            }

            if (long_format) {
                print_lf_details(path, show_all);
            } else {
                DIR *dir = opendir(path);
                if (!dir) {
                    printf("%sErro ao abrir %s%s\n", TERM_RED_BOLD, path, TERM_RESET);
                    last_command_exit_error = true;
                    continue;
                }

                struct dirent **entries;
                int n = scandir(path, &entries, NULL, alphasort);

                for (int i = 0; i < n; i++) {
                    if (!show_all && entries[i]->d_name[0] == '.') {
                        free(entries[i]);
                        continue;
                    }

                    char full_path[2048];
                    if (strlen(path) + strlen(entries[i]->d_name) + 1 < sizeof(full_path)) {
                        snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i]->d_name);
                    } else {
                        fprintf(stderr, "Warning: Path too long, skipping entry '%s'\n", entries[i]->d_name);
                        continue;
                    }

                    struct stat st;
                    if (stat(full_path, &st) == 0) {
                        const char *color = S_ISDIR(st.st_mode) ? TERM_BLUE : TERM_GREEN;
                        printf("%s%s%s\n", color, entries[i]->d_name, TERM_RESET);
                    }
                    free(entries[i]);
                }
                free(entries);
                closedir(dir);
            }
            last_command_exit_error = false;
        } else if (strcmp(args[0], "tree") == 0) {
            if (arg_count < 2) {
                printf("%sPID faltando%s\n", TERM_RED_BOLD, TERM_RESET);
                last_command_exit_error = true;
                continue;
            }
            char *pid_str = args[1];
            if (!is_number(pid_str)) {
                printf("%sPID inválido%s\n", TERM_RED_BOLD, TERM_RESET);
                last_command_exit_error = true;
                continue;
            }
            pid_t pid = atoi(pid_str);
            struct process_info info = get_process_info(pid);
            if (info.pid == 0) {
                printf("%sProcesso %d não encontrado%s\n", TERM_RED_BOLD, pid, TERM_RESET);
                last_command_exit_error = true;
                continue;
            }
            bool ancestors[16] = {0}; // Assume profundidade máxima de 16
            printf("Árvore de processos (PID %d):\n", pid);
            print_process_tree(pid, 0, true, ancestors);
        } else {
            // Comando externo
            pid_t pid = fork();
            if (pid == 0) {
                execvp(args[0], args);
                fprintf(stderr, "%sErro ao executar %s%s\n", TERM_RED_BOLD, args[0], TERM_RESET);
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                fprintf(stderr, "%sErro ao criar processo%s\n", TERM_RED_BOLD, TERM_RESET);
                last_command_exit_error = true;
            } else {
                if (!background) {
                    int status;
                    waitpid(pid, &status, 0);
                    last_command_exit_error = (WEXITSTATUS(status) != 0);
                } else {
                    printf("[%d] Executando em segundo plano\n", pid);
                    last_command_exit_error = false;
                }
            }
        }
    }

    free(_PATH);
    return 0;
}

// Função para obter informações do processo
struct process_info get_process_info(pid_t pid) {
    struct process_info info = {0};
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    FILE *f = fopen(stat_path, "r");
    if (!f) return info;

    char line[1024];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return info;
    }
    fclose(f);

    char *start = strchr(line, '(');
    char *end = strrchr(line, ')');
    if (!start || !end) return info;

    *end = '\0';
    info.pid = pid;
    strncpy(info.name, start + 1, sizeof(info.name) - 1);

    char *remaining = end + 1;
    sscanf(remaining, "%*s %d", &info.ppid);

    return info;
}

// Função recursiva para imprimir a árvore de processos
void print_process_tree(const pid_t pid, const int depth, const bool is_last, const bool *ancestors) {
    struct process_info info = get_process_info(pid);
    if (info.pid == 0) return;

    // Cores por nível
    const char *colors[] = {TERM_CYAN, TERM_GREEN, TERM_MAGENTA, TERM_BLUE, TERM_YELLOW};
    const char *color = colors[depth % 5];

    // Imprimir indentação
    for (int i = 0; i < depth; i++) {
        if (i == depth - 1) {
            printf(is_last ? "└── " : "├── ");
        } else {
            printf(ancestors[i] ? "│   " : "    ");
        }
    }

    // Imprimir processo atual
    printf("%s%s (PID: %d)%s\n", color, info.name, pid, TERM_RESET);

    // Coletar filhos
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return;

    pid_t children[MAX_PROCESSES];
    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(proc_dir))) {
        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "/proc/%s", entry->d_name);
        struct stat st;
        if (lstat(full_path, &st) == 0 && S_ISDIR(st.st_mode) && is_number(entry->d_name)) {
            pid_t child_pid = atoi(entry->d_name);
            struct process_info child_info = get_process_info(child_pid);
            if (child_info.ppid == pid) {
                children[count++] = child_pid;
            }
        }
    }
    closedir(proc_dir);

    // Ordenar filhos por PID
    qsort(children, count, sizeof(pid_t), compare_pids);

    // Imprimir filhos recursivamente
    bool new_ancestors[depth + 1];
    memcpy(new_ancestors, ancestors, depth * sizeof(bool));

    for (int i = 0; i < count; i++) {
        new_ancestors[depth] = (i != count - 1);
        print_process_tree(children[i], depth + 1, (i == count - 1), new_ancestors);
    }
}

// Handler para SIGCHLD (evitar zumbis)
void sigchld_handler(int sig) {
    (void) sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void welcome_message() {
    // printf("%s┍%.*s┑%s\n", TERM_CYAN, (int)TERM_WIDTH, "━", TERM_RESET);
    printf("%s┍", TERM_CYAN);
    for (size_t i = 0; i < TERM_WIDTH - 2; i++) {
        printf("━");
    }
    printf("┑%s\n", TERM_RESET);

    char *msg = "WELCOME TO GREGORIOUS SHELL";
    const int spaces = (int) (TERM_WIDTH - strlen(msg) - 2) / 2;
    printf("%s│%*s%s%s%s%*s%s\n", TERM_CYAN, spaces, "", TERM_YELLOW, msg, TERM_CYAN, spaces + 1, "", "│");
    printf("%s┕", TERM_CYAN);
    for (size_t i = 0; i < TERM_WIDTH - 2; i++) {
        printf("━");
    }
    printf("┙%s\n", TERM_RESET);
    printf("%s\n", TERM_GREEN);
    printf("Type 'help' for a list of commands.\n");
    printf("Type 'exit' to exit the shell.\n");
    printf("%s\n", TERM_RESET);
}

void print_lf_details(const char *path, bool show_all) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent **entries;
    int n = scandir(path, &entries, NULL, alphasort);

    setlocale(LC_NUMERIC, ""); // Para separadores de milhares

    printf("%s%-13.11s%-12s%-12s%9.8s %-13s%s%s\n",
           TERM_YELLOW, "Permissões", "Dono", "Grupo", "Tamanho",
           "Modificado", "Nome", TERM_RESET
    );

    for (int i = 0; i < n; i++) {
        if (!show_all && entries[i]->d_name[0] == '.') {
            free(entries[i]);
            continue;
        }

        char full_path[2048 + 257];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i]->d_name);

        struct stat st;
        if (lstat(full_path, &st)) {
            free(entries[i]);
            continue;
        }

        // Permissões
        char perms[11];
        mode_to_str(st.st_mode, perms);

        // Dono/Grupo
        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);

        // Tamanho formatado
        char size_str[32];
        strcpy(size_str, human_readable_size(st.st_size));

        // Data
        char date[20];
        strftime(date, sizeof(date), "%d %b %H:%M", localtime(&st.st_mtime));

        // Cores
        const char *file_color = TERM_WHITE;
        if (S_ISDIR(st.st_mode)) file_color = TERM_BLUE_BOLD;
        else if (st.st_mode & S_IXUSR) file_color = TERM_GREEN_BOLD;
        else if (S_ISLNK(st.st_mode)) file_color = TERM_MAGENTA_BOLD;

        const char *owner_color = TERM_WHITE;
        if (pw && strcmp(pw->pw_name, "root") == 0) owner_color = TERM_RED_BOLD;
        else if (pw && strcmp(pw->pw_name, "?") != 0) owner_color = TERM_BLUE_BOLD;

        const char *group_color = TERM_WHITE;
        if (gr && !strcmp(gr->gr_name, "root") == 0) owner_color = TERM_RED_BOLD;
        else if (gr && strcmp(gr->gr_name, "?") != 0) owner_color = TERM_MAGENTA_BOLD;


        // "%s%-13.11s%-12s%-12s%9.8s %-13s%s%s\n",
        printf("%s%-12.11s%s%-12s%s%-12s%s%9.8s %s%-13s%s%s%s%s\n",
               TERM_WHITE_BOLD, perms,
               owner_color, pw ? pw->pw_name : "?",
               group_color, gr ? gr->gr_name : "?",
               TERM_RESET, size_str,
               TERM_CYANBRIGHT, date,
               file_color, entries[i]->d_name,
               S_ISLNK(st.st_mode) ? "@" : "",
               TERM_RESET
        );
        free(entries[i]);
    }
    free(entries);
    closedir(dir);
}

bool is_number(const char *str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (!isdigit(str[i])) return false;
    }
    return true;
}

void strmode(mode_t mode, char *str) {
    str[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : '-';
    for (int i = 0; i < 9; i++) {
        const char *chars = "rwxrwxrwx";
        str[i + 1] = (mode & (1 << (8 - i))) ? chars[i] : '-';
    }
    str[10] = '\0';
}

int compare_pids(const void *a, const void *b) {
    return (*(pid_t *) a - *(pid_t *) b);
}

static void mode_to_str(mode_t mode, char *str) {
    str[0] = (mode & S_IFDIR) ? 'd' : (mode & S_IFLNK) ? 'l' : '-';
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

static char *human_readable_size(long bytes) {
    static char buf[32];
    char units[] = "BKMGTP";
    int i = 0;
    double size = bytes;

    while (size >= 1024 && units[i + 1]) {
        size /= 1024;
        i++;
    }

    snprintf(buf, sizeof(buf), "%.1f%c", size, units[i]);
    return buf;
}


// v1.0 (Apr 11 2025 - 10:44) - Creating the concept
// v1.0.1 (Apr 11 2025 - 11:02) - Creating the logic of fork
// v1.0.2 (Apr 11 2025 - 11:08) - Creating `cd` command
// v1.0.3 (Apr 11 2025 - 11:20) - Creating `tree` command
// v1.0.4 (Apr 11 2025 - 11:56) - Creating `ls` command
// v1.1.0 (Apr 11 2025 - 12:05) - "It's not easy, right" Found a bug in `tree` command
// v1.1.2 (Apr 11 2025 - 12:13) - "It's just a pointer!!!!" Solved the bug
// v1.1.3 (Apr 11 2025 - 12:13) - Improving the draw in tree command (check: https://elixir.bootlin.com/busybox/1.37.0/source/procps/pstree.c)
// v1.2.0 (Apr 11 2025 - 12:31) - COLOR IN EVERYTHING HAHAHAHA.
// v1.2.1 (Apr 11 2025 - 12:38) - Improvements in ls command, now he accepts args (-l -a -la -al)
// v1.3.0 (Apr 11 2025 - 13:11) - Refactor in mode_to_str, print_ls_detail, human_readable_size
// v1.3.1 (Apr 11 2025 - 13:44) - Review in extra funtions (removed is_last_child)
// v1.3.2 (Apr 11 2025 - 14:26) - Refactor in auxiliary functions
// v1.3.3 (Apr 11 2025 - 14:51) - Creating the code documentation
// v1.3.4 (Apr 11 2025 - 15:19) - Segmentation Fault in ls command without params | __LINE__ 261 Solved with arg_count > 1
// v1.3.5 (Apr 11 2025 - 15:47) - Code Documentation, OK
// v1.3.6 (Apr 11 2025 - 16:00) - Removing unnecessary code, adding colors to ls -h
// v1.4.0 (Apr 15 2025 - 10:03) - Professor has request to change command ls to another name, changed to lf
// v1.4.1 (Apr 15 2025 - 10:22) - Fix trunk warning at __LINE__ 260
// v1.4.2 (Apr 22 2025 - 09:48) - Clean the terminal before showing the prompt
// v1.4.3 (Apr 22 2025 - 09:51) - Fix the warning at __LINE__ 463
