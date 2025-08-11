#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ---------- Trim function ----------
char *trim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
    return str;
}

// ---------- getline & strndup for Windows ----------
#if !defined(_SSIZE_T_DEFINED) && !defined(_SSIZE_T)
typedef long ssize_t;
#define _SSIZE_T_DEFINED
#endif

ssize_t my_getline(char **lineptr, size_t *n, FILE *stream) {
    char *buf;
    int c;
    size_t len = 0;

    if (*lineptr == NULL) {
        *n = 128;
        *lineptr = malloc(*n);
        if (*lineptr == NULL) return -1;
    }
    buf = *lineptr;

    while ((c = fgetc(stream)) != EOF) {
        if (len + 1 >= *n) {
            *n *= 2;
            buf = realloc(buf, *n);
            if (buf == NULL) return -1;
            *lineptr = buf;
        }
        buf[len++] = c;
        if (c == '\n') break;
    }
    buf[len] = '\0';
    return (len > 0) ? len : -1;
}

char *my_strndup(const char *s, size_t n) {
    char *result;
    size_t len = strlen(s);
    if (len > n) len = n;
    result = (char *)malloc(len + 1);
    if (!result) return NULL;
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

#define getline my_getline
#define strndup my_strndup

// ---------- Variable storage ----------
typedef struct {
    char name[50];
    int value;
} Var;

Var vars[100];
int varCount = 0;

void setVar(const char *name, int value) {
    char temp[50];
    strcpy(temp, name);
    trim(temp);
    
    // Check if variable already exists
    for (int i = 0; i < varCount; i++) {
        if (strcmp(vars[i].name, temp) == 0) {
            vars[i].value = value;
            return;
        }
    }
    
    // Add new variable
    strcpy(vars[varCount].name, temp);
    vars[varCount].value = value;
    varCount++;
}

int getVar(const char *name) {
    char temp[50];
    strcpy(temp, name);
    trim(temp);
    
    for (int i = 0; i < varCount; i++) {
        if (strcmp(vars[i].name, temp) == 0) {
            return vars[i].value;
        }
    }
    printf("QutixBrain program executed successfully!\n");
    return 0;
}

// ---------- Execute one line ----------
void executeLine(char *line) {
    line = trim(line);
    if (strlen(line) == 0) return;

    // print statement
    if (strncmp(line, "print ", 6) == 0) {
        char *arg = trim(line + 6);
        if (arg[0] == '"' && arg[strlen(arg) - 1] == '"') {
            // String literal
            arg[strlen(arg) - 1] = '\0';
            printf("%s\n", arg + 1);
        } else {
            // Variable
            printf("%d\n", getVar(arg));
        }
    }
    // let statement: let x = 5
    else if (strncmp(line, "let ", 4) == 0) {
        char name[50];
        int value;
        if (sscanf(line + 4, "%49s = %d", name, &value) == 2) {
            setVar(name, value);
        }
    }
    // if statement
    else if (strncmp(line, "if ", 3) == 0) {
        char var[50], op[3];
        int num;
        char *then_pos = strstr(line, " then ");
        if (then_pos != NULL) {
            // Parse condition part
            char condition[100];
            int cond_len = then_pos - (line + 3);
            strncpy(condition, line + 3, cond_len);
            condition[cond_len] = '\0';
            
            if (sscanf(condition, "%49s %2s %d", var, op, &num) == 3) {
                int v = getVar(var);
                int cond = 0;
                if (strcmp(op, ">") == 0) cond = (v > num);
                else if (strcmp(op, "<") == 0) cond = (v < num);
                else if (strcmp(op, "==") == 0) cond = (v == num);

                if (cond) {
                    executeLine(then_pos + 6); // Execute after "then "
                }
            }
        }
    }
    // for loop - Fixed parsing
    else if (strncmp(line, "for ", 4) == 0) {
        char var[50];
        int start, end;
        char *brace_pos = strchr(line, '{');
        
        if (brace_pos != NULL) {
            // Parse with braces: for i = 1 to 5 { print i }
            char for_part[100];
            int for_len = brace_pos - (line + 4);
            strncpy(for_part, line + 4, for_len);
            for_part[for_len] = '\0';
            
            if (sscanf(for_part, "%49s = %d to %d", var, &start, &end) == 3) {
                // Extract body (everything after '{' and before '}')
                char *body_start = brace_pos + 1;
                char *body_end = strrchr(line, '}');
                if (body_end != NULL) {
                    int body_len = body_end - body_start;
                    char body[200];
                    strncpy(body, body_start, body_len);
                    body[body_len] = '\0';
                    
                    for (int i = start; i <= end; i++) {
                        setVar(var, i);
                        executeLine(trim(body));
                    }
                }
            }
        } else {
            // Parse without braces (for simpler syntax)
            char rest_of_line[200];
            if (sscanf(line + 4, "%49s = %d to %d %199[^\n]", var, &start, &end, rest_of_line) == 4) {
                for (int i = start; i <= end; i++) {
                    setVar(var, i);
                    executeLine(rest_of_line);
                }
            }
        }
    }
}

// ---------- File extension validation ----------
int hasQbitExtension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;
    return (strcmp(dot, ".qbit") == 0);
}

// ---------- Main ----------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("QutixBrain Language Interpreter\n");
        printf("Usage: %s <sourcefile.qbit>\n", argv[0]);
        printf("File must have .qbit extension\n");
        return 1;
    }

    // Check file extension
    if (!hasQbitExtension(argv[1])) {
        printf("Error: Invalid file extension. QutixBrain files must have .qbit extension\n");
        printf("Example: myprogram.qbit\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Error: Cannot open QutixBrain file '%s'\n", argv[1]);
        perror("Details");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fp) != -1) {
        executeLine(line);
    }
    free(line);
    fclose(fp);
    return 0;
}