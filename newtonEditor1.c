#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<errno.h>
#include<termios.h>
#include<unistd.h>
#include<fcntl.h>

struct termios original, raw;

// function for disabling terminal modes.
int enableTerminalRaw();

// function for disabling terminal raw modes.
int disableTerminalRaw();

// function for clearing terminal.
int clearTerminal();

// function for printing screen in the terminal.
int printingScreen();

// function for inputing texts.
int Input();

int main(){
    // defining variables for input from terminal to start the editor.

    char* input = malloc(1000);
    input[0] = '\0';
    char* fileName = malloc(1000);
    fileName[0] = '\0';
    char* cmd = malloc(1000);
    cmd[0] = '\0';

    // taking input from the user as filename and commands.

    fgets(input, 1000, stdin);
    sscanf(input, "%999s %999s", cmd, fileName);

    // here starts the original code.

    if(strcmp(cmd, "text") == 0){
        // opening the file named fileName and if it doesnot exist.
        // than creating the file and opening.

        int file = open(fileName, O_RDWR | O_CREAT, 0644);
        if(file < 0){
            perror("Can't open file: ");
            return 0;
        }
        
        // defining buffer for reading file contents and user text input.

        size_t capacity = 100000;
        size_t contents = 0;
        char* buffer = malloc(capacity);
        buffer[0] = '\0';

        // enabling terminal raw modes so that they can't intercept in the editor processes.

        enableTerminalRaw();

        // reading file contents.
        
        ssize_t n = read(file, buffer, capacity -1 );
        if(n < 0){
            perror("Read: ");
            return 1;
        }
        contents = (size_t)n;
        buffer[contents] = '\0';

        // defining cursor properties.

        size_t cursor = contents;
        size_t row = 0;
        size_t col = 0;

        size_t prefCol = 0;
        write(STDOUT_FILENO, buffer, contents);
        char seq[32];
        snprintf(seq, sizeof(seq), "\x1b[%zu;%zuH", row + 1, col +1);
        write(STDOUT_FILENO, seq, strlen(seq));

        // starting editing part.

        while(1){
            row = 0;
            col = 0;
            for(size_t i=0; i < cursor; i++){
                if(buffer[i] == '\n'){
                    row++;
                    col =0;
                }
                else{
                    col++;
                }
            }

            // clear the whole terminal.

            clearTerminal();

            // reprinting screen.

            printingScreen();

            // defining buffers and taking text inputs.

            Input();
        }
    }
    disableTerminalRaw();
}

int enableTerminalRaw(){
    tcgetattr(STDIN_FILENO, &original);
    raw = original;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_iflag &= ~(IXON | ICRNL);
    int chechTCS = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if(chechTCS == -1){
        perror("tcsetattr: ");
        return 0;
    }
}

int disableTerminalRaw(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

int clearTerminal(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

int printingScreen(){
    // printing contents continuously.
            
    write(STDOUT_FILENO, buffer, contents);

    // reprinting screen. 

    snprintf(seq, sizeof(seq), "\x1b[%zu;%zuH", row + 1, col +1);
    write(STDOUT_FILENO, seq, strlen(seq));
}

int Input(){
    char readChar;
            
    if(read(STDIN_FILENO, &readChar, 1) != 1){
        continue;
    }

    if(readChar == '\x1b'){
        char  seq[2];

        if(read(STDIN_FILENO, &seq[0], 1) != 1){
            continue;
        }

        if(read(STDIN_FILENO, &seq[1], 1) != 1){
            continue;
        }

        // positioning cursor.

        if(seq[0] == '['){
            switch (seq[1]){
                case 'D':
                    if(cursor > 0){
                        cursor--;
                    }
                    break;

                case 'C':
                    if(cursor < contents){
                        cursor++;
                    }
                    break;
                    
                case 'A': {
                        
                    if(row > 0){
                        size_t currStart = cursor;
                        while(currStart > 0 && buffer[currStart - 1] != '\n'){
                            currStart--;
                        }

                        size_t prevEnd = currStart - 1;

                        size_t prevStart = prevEnd;

                        while(prevStart > 0 && buffer[prevStart - 1] != '\n'){
                            prevStart--;
                        }

                        cursor = prevStart;

                        size_t x = 0;

                        while(x < prefCol && cursor < prevEnd){
                            cursor++;
                            x++;
                        }
                    }

                    break;
                }
                case 'B': {
                    size_t currEnd = cursor;

                    while(currEnd < contents && buffer[currEnd] != '\n'){
                        currEnd++;
                    }

                    if(currEnd == contents){
                        break;
                    }
                    size_t nextStart = currEnd + 1;
                    cursor = nextStart;
                    size_t x = 0;
                    while(x < prefCol && cursor < contents && buffer[cursor] !='\n'){
                        cursor++;
                        x++;
                    }
                    break;
                }
            }

            continue;

            }
        }

        // if ctrl+q is pressed it will stop the program.

        if(readChar == 17){
            break;
        }

        // if buffer is full, then new memory is re-allocated.
        if(contents + 1 >= capacity){
            capacity *= 2;
            char* tempMem = realloc(buffer, capacity);
            // checking if memory allocation is ok or not.

            if(tempMem == NULL){
                perror("Can't allocate new memory: ");
                break;
            }

            buffer = tempMem;

        }

        // taking real text input.

        if(readChar >= 32 && readChar <= 126){
            memmove(buffer + cursor +1,
                    buffer + cursor, 
                    contents - cursor + 1);
                
            buffer[cursor] = readChar;

            cursor++;
            contents++;

            prefCol = col + 1;

            buffer[contents] = '\0';
        }

        // code for using backspace.

        if(readChar == 127 && cursor > 0){
            memmove(buffer + cursor -1,
                    buffer + cursor,
                    contents - cursor + 1);

            cursor--;
            contents--;

            prefCol = col - 1;

            buffer[contents] = '\0';
        }

        if(readChar == '\r' || readChar == '\n'){
            memmove(buffer + cursor +1,
                    buffer + cursor,
                    contents - cursor + 1);
                    
            buffer[cursor] = '\n';

            cursor++;
            contents++;

            prefCol = 0;

            buffer[contents] = '\0';
        }

        if(readChar == 15){
            write(STDOUT_FILENO, "saving... \n", 10);

            lseek(file, 0, SEEK_SET);
            ftruncate(file, 0);
            write(file, buffer, contents);
            fsync(file);
            continue;
        }
    }
}
int summetion(a, b){
    return a+b;
}