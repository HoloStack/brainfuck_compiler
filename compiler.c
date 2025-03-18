#include <stdio.h>
#include <stdlib.h>

void generate_x86(FILE *input, FILE *output) {
    fprintf(output, "section .bss\n"
                    "    tape resb 30000\n\n"
                    "section .text\n"
                    "global _start\n\n"
                    "_start:\n"
                    "    mov esi, tape\n");

    int loop_id = 0;
    int loop_stack[100];
    int loop_stack_top = 0;
    
    char c;
    while ((c = fgetc(input)) != EOF) {
        switch (c) {
            case '>': fprintf(output, "    inc esi\n"); break;
            case '<': fprintf(output, "    dec esi\n"); break;
            case '+': fprintf(output, "    inc byte [esi]\n"); break;
            case '-': fprintf(output, "    dec byte [esi]\n"); break;
            case '.':
                fprintf(output, "    movzx eax, byte [esi]\n"
                                "    call putchar\n");
                break;
            case ',':
                fprintf(output, "    call getchar\n"
                                "    mov [esi], al\n");
                break;
            case '[':
                loop_stack[loop_stack_top++] = loop_id;
                fprintf(output, "loop_start_%d:\n"
                                "    cmp byte [esi], 0\n"
                                "    jz loop_end_%d\n", loop_id, loop_id);
                loop_id++;
                break;
            case ']':
                loop_id = loop_stack[--loop_stack_top];
                fprintf(output, "    cmp byte [esi], 0\n"
                                "    jnz loop_start_%d\n"
                                "loop_end_%d:\n", loop_id, loop_id);
                break;
        }
    }

    // Exit
    fprintf(output, "    mov eax, 60\n"
                    "    xor edi, edi\n"
                    "    syscall\n");
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
    FILE *output = fopen(output_filename, "w");

    if (!input) {
        perror("Error opening input file");
        return 1;
    }
    if (!output) {
        perror("Error opening output file");
        fclose(input);
        return 1;
    }

    generate_x86(input, output);
    
    fclose(input);
    fclose(output);
    printf("Assembly code generated: %s\n", output_filename);
    return 0;
}