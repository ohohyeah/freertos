#define USE_STDPERIPH_DRIVER
#include "stm32f10x.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"

extern const char _sromfs;

static void setup_hardware();

volatile xQueueHandle serial_str_queue = NULL;
volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
volatile xQueueHandle serial_rx_queue = NULL;

/* Queue structure used for passing messages. */
typedef struct {
	char str[100];
} serial_str_msg;

/* Queue structure used for passing characters. */
typedef struct {
	char ch;
} serial_ch_msg;

void queue_str_task(const char *str, int delay)
{
	serial_str_msg msg;

	/* Prepare the message to be queued. */
	strcpy(msg.str, str);

	while (1) {
		/* Post the message.  Keep on trying until it is successful. */
		while (!xQueueSendToBack(serial_str_queue, &msg,
		       portMAX_DELAY));

		/* Wait. */
		vTaskDelay(delay);
	}
}

/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
	serial_ch_msg rx_msg;
	static signed portBASE_TYPE xHigherPriorityTaskWoken;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		
	}
	/* If this interrupt is for a receive... */
	else if  (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		/* Receive the byte from the buffer. */
		rx_msg.ch = USART_ReceiveData(USART2);

		/* Queue the received byte. */
		if(!xQueueSendToBackFromISR(serial_rx_queue, &rx_msg, &xHigherPriorityTaskWoken)) {
			/* If there was an error queueing the received byte,
			 * freeze. */
			while(1);	
		}	
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART2, ch);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

char receive_byte()
{
	serial_ch_msg msg;

	/* Wait for a byte to be queued by the receive interrupts handler. */
	while (!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));

	return msg.ch;
}


void read_romfs_task(void *pvParameters)
{
	char buf[128];
	size_t count;
	int fd = fs_open("/romfs/test.txt", 0, O_RDONLY);
	do {
		//Read from /romfs/test.txt to buffer
		count = fio_read(fd, buf, sizeof(buf));
		
		//Write buffer to fd 1 (stdout, through uart)
		fio_write(1, buf, count);
	} while (count);
	
	while (1);
}


void rs232_xmit_msg_task(void *pvParameters)
{
	serial_str_msg msg;
	int curr_char;

	while (1) {
		/* Read from the queue.  Keep trying until a message is
		 * received.  This will block for a period of time (specified
		 * by portMAX_DELAY). */
		while (!xQueueReceive(serial_str_queue, &msg, portMAX_DELAY));

		/* Write each character of the message to the RS232 port. */
		curr_char = 0;
		while (msg.str[curr_char] != '\0') {
			send_byte(msg.str[curr_char]);
			curr_char++;
		}
	}
}

void serial_readwrite_task(void *pvParameters)
{
	serial_str_msg msg;
	char ch;
	int curr_char;
	int done;

	/* Prepare the response message to be queued. */
	strcpy(msg.str, "Got:");

	while (1) {
		curr_char = 4;
		done = 0;
		do {
			/* Receive a byte from the RS232 port (this call will
			 * block). */
			ch = receive_byte();

			/* If the byte is an end-of-line type character, then
			 * finish the string and inidcate we are done.
			 */
			if ((ch == '\r') || (ch == '\n')) {
				msg.str[curr_char] = '\n';
				msg.str[curr_char+1] = '\0';
				done = -1;
				/* Otherwise, add the character to the
				 * response string. */
			}
			else {
				msg.str[curr_char++] = ch;
			}
		} while (!done);

		/* Once we are done building the response string, queue the
		 * response to be sent to the RS232 port.
		 */
		
		while (!xQueueSendToBack(serial_str_queue, &msg,
		                         portMAX_DELAY));
	}
}



int main()
{
	init_rs232();
	enable_rs232_interrupts();
	enable_rs232();
	
	fs_init();
	fio_init();

	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	serial_str_queue = xQueueCreate(10, sizeof(serial_str_msg));
	vSemaphoreCreateBinary(serial_tx_wait_sem);
	serial_rx_queue = xQueueCreate(1, sizeof(serial_ch_msg));
	
//	register_romfs("romfs", &_sromfs);
	
	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);

	/* Create a task to  read from romfs. */
	//xTaskCreate(read_romfs_task,
	  //          (signed portCHAR *) "Read romfs",
	    //        512 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);


	/* Create a task to write messages from the queue to the RS232 port. */
	xTaskCreate(rs232_xmit_msg_task,
	            (signed portCHAR *) "Serial Xmit Str",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);
	
	/* Create a task to receive characters from the RS232 port and echo
	 * them back to the RS232 port. */
	xTaskCreate(serial_readwrite_task,
	            (signed portCHAR *) "Serial Read/Write",
	            512 /* stack size */, NULL,
	            tskIDLE_PRIORITY + 10, NULL);

	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
