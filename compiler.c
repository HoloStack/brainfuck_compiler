#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_NESTED_LOOPS 100  
void generate_x86_64(FILE *input, FILE *output) {
    fprintf(output,
        "section .bss\n"
        "    tape resb 30000\n\n"
        "section .text\n"
        "global _start\n\n"
        "_start:\n"
        "    mov rsi, tape\n");

    int loop_stack[MAX_NESTED_LOOPS];  // Stack for loop IDs
    int loop_stack_top = 0;
    int loop_id = 0;
    
    char c;
    while ((c = fgetc(input)) != EOF) {
        switch (c) {
            case '>': fprintf(output, "    inc rsi\n"); break;
            case '<': fprintf(output, "    dec rsi\n"); break;
            case '+': fprintf(output, "    inc byte [rsi]\n"); break;
            case '-': fprintf(output, "    dec byte [rsi]\n"); break;
            case '.':
                fprintf(output,
                        "    mov rax, 1\n"
                        "    mov rdi, 1\n"
                        //"    mov rsi, rsi\n" redundent
                        "    mov rdx, 1\n"
                        "    syscall\n");
                break;
            case ',':
                fprintf(output,
                        "    mov rax, 0\n"
                        "    mov rdi, 0\n" 
                        //"    mov rsi, rsi\n" redundent
                        "    mov rdx, 1\n"
                        "    syscall\n");
                break;
            case '[':
                if (loop_stack_top >= MAX_NESTED_LOOPS) {
                    fprintf(stderr, "Error: Too many nested loops\n");
                    exit(1);
                }
                loop_stack[loop_stack_top++] = loop_id;
                fprintf(output, 
                        "loop_start_%d:\n"
                        "    cmp byte [rsi], 0\n"
                        "    je loop_end_%d\n", loop_id, loop_id);
                loop_id++;
                break;
            case ']':
                if (loop_stack_top == 0) {
                    fprintf(stderr, "Error: Unmatched ']'\n");
                    exit(1);
                }
                int id = loop_stack[--loop_stack_top];
                fprintf(output, 
                        "    cmp byte [rsi], 0\n"
                        "    jne loop_start_%d\n"
                        "loop_end_%d:\n", id, id);
                break;
        }
    }

    if (loop_stack_top > 0) {
        fprintf(stderr, "Error: Unmatched '['\n");
        exit(1);
    }

    // Exit syscall
    fprintf(output, 
            "    mov rax, 60\n"
            "    xor rdi, rdi\n"
            "    syscall\n");
}

void assemble_and_run(const char *assembly_file) {
    char obj_file[256], exe_file[256], ld_cmd[512];

    snprintf(obj_file, sizeof(obj_file) - 1, "%.200s.o", assembly_file);
    snprintf(exe_file, sizeof(exe_file) - 1, "%.200s_exec", assembly_file);
    
    int written = snprintf(ld_cmd, sizeof(ld_cmd) - 1, "ld %s -o %s", obj_file, exe_file);
    if (written >= sizeof(ld_cmd)) {
        fprintf(stderr, "Warning: ld command truncated!\n");
    }

    char choice;
    printf("Do you want to assemble and run the program? (y/n): ");
    scanf(" %c", &choice);

    if (choice == 'y' || choice == 'Y') {
        char nasm_cmd[512];
        snprintf(nasm_cmd, sizeof(nasm_cmd) - 1, "nasm -f elf64 %s -o %s", assembly_file, obj_file);

        printf("Assembling: %s\n", nasm_cmd);
        if (system(nasm_cmd) != 0) {
            fprintf(stderr, "Error assembling %s\n", assembly_file);
            exit(1);
        }

        printf("Linking: %s\n", ld_cmd);
        if (system(ld_cmd) != 0) {
            fprintf(stderr, "Error linking %s\n", obj_file);
            exit(1);
        }

        printf("Running: %s\n", exe_file);
        char run_cmd[300];
        snprintf(run_cmd, sizeof(run_cmd) - 1, "./%s", exe_file);
        system(run_cmd);
    } else {
        printf("Skipping assembly and execution.\n");
    }
}

int main(int argc, char *argv[]) {
    char input_filename[256], output_filename[256];

    if (argc < 3) {
        printf("Enter Brainfuck source file: ");
        scanf("%255s", input_filename);
        printf("Enter output assembly file: ");
        scanf("%255s", output_filename);
    } else {
        snprintf(input_filename, sizeof(input_filename), "%s", argv[1]);
        snprintf(output_filename, sizeof(output_filename), "%s", argv[2]);
    }

    FILE *input = fopen(input_filename, "r");
    if (!input) {
        perror("Error opening input file");
        return 1;
    }

    FILE *output = fopen(output_filename, "w");
    if (!output) {
        perror("Error opening output file");
        fclose(input);
        return 1;
    }

    generate_x86_64(input, output);

    fclose(input);
    fclose(output);
    printf("Assembly code generated: %s\n", output_filename);

    assemble_and_run(output_filename);

    return 0;
}
