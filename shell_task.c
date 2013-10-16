
#include "fio.h"



/* define */
#define MAX_MSG_LEN 100
#define BACKSPACE 127
#define MAX_COMM_COUNT 2
#define MAX_COMM_PARA 5
char newline[3] = {'\r','\n','\0'};
char backspace[3] = {'\b',' ','\b'};


/* custom structs */
typedef void func_handler(int , char*[]);

typedef struct cmd_type {
       char *cmd;
       func_handler *handler;
       char *description;
       } cmd_type;
static cmd_type commands[MAX_COMM_COUNT];

/* co*/
void helpmenu(int argc, char* argv[])
{
	myprintf("===== avaliable commands =====%s", newline);
	int i ;
	for(i=0 ; i <MAX_COMM_COUNT ; i++)
	{
		myprintf("%s\t%s%s",commands[i].cmd, commands[i].description , newline);
	}
	myprintf("==============================%s", newline);
}


static cmd_type commands[MAX_COMM_COUNT] =
{
        {.cmd = "help", .handler = helpmenu, .description = "show all avaliable commands."},
        {.cmd = "echo", .handler = "ff", .description = "repeat what you type in."},  
};       




void cmd_parser(char *str,int argc, char *argv[]){
   
        int i;
        int count=0, p=0;
        for(i=0; str[i]; ++i){

                if(str[i]==' ')
		{
                        str[i]='\0';
                        argv[argc++]=&str[p];
                        p=i+1;
                }
        }
        argv[count++]=&str[p];
}


int strcmpZ(const char *str_a, const char *str_b)
{
    int i = 0;

    while(str_a[i]) {
        if (str_a[i] != str_b[i]) {
            return str_a[i] - str_b[i];
        }
        i++;
    }
    return 0;
}

void cmd_handler(int argc , char *argv[])
{
	int i ;
	for(i=0 ; i <MAX_COMM_COUNT ; i++)
	{
	
		if (strcmp(commands[i].cmd, argv[0])==0)	
		{
			commands[i].handler(argc, argv);
			break;
		}
	}

}


void shell_task(void *pvParameters)
{
	char msg[MAX_MSG_LEN];
	char ch;
	char *p_argv[MAX_COMM_PARA];
	int curr_char;
	int done;
	int p_argc=0;
	/* Prepare the response message to be queued. */

	while (1) {
		curr_char = 0;
		done = 0;
		myprintf("shell>");
		
		
		do {
			/* Receive a byte from the RS232 port (this call will
			 * block). */
			ch = receive_byte();
			
			/* If the byte is an end-of-line type character, then
			 * finish the string and inidcate we are done.
			 */
			if ((ch == '\r') || (ch == '\n')|| (curr_char > MAX_MSG_LEN) ) 
			{
				msg[curr_char] = '\0';
				done = -1;
				myprintf(newline);
				cmd_parser(msg , p_argc, p_argv);
				cmd_handler(p_argc, p_argv);
				myprintf(p_argv[1]);
			}			
			else if(ch == "\b" || ch== BACKSPACE )
			{
				curr_char--;
				myprintf(backspace);
			}
			else {
				msg[curr_char++] = ch;			
				send_byte(ch);
				//for test				
				//fio_write(1, &ch ,1);
			}
		} while (!done);
		
		
	}
}





