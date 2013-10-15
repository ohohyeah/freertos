/* define */
#define MAX_MSG_LEN 100
#define BACKSPACE 127;
char newline[3] = {'\r','\n','\0'};
char backspace[3] = {'\b',' ','\b'};


/*command struct */
typedef struct cmd_type {
       char *cmd;
       char *handler;
       char *description;
       } cmd_type;
       
 cmd_type commands[10] =
{
        {.cmd = "help", .handler = "ff", .description = "show all commands."},
        {.cmd = "echo", .handler = "ff", .description = "repeat what you type in."},  
};


void shell_task(void *pvParameters)
{
	char msg[MAX_MSG_LEN];
	char ch;
	int curr_char;
	int done;

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
			if ((ch == '\r') || (ch == '\n')) 
			{
				msg[curr_char] = '\0';
				done = -1;
				myprintf(newline);
			}			
			else if(ch == "\b" || ch==127 )
			{
				curr_char--;
				myprintf(backspace);
			}
			else {
				msg[curr_char++] = ch;			
				send_byte(ch);
			}
		} while (!done);
		
		
	}
}


