#include "os_tasks.h"
#include "funcs.h"

// modify output buffer according to character
// clear current line
// re-print output buffer


void handleInputChar(char c) {
	// modify output buffer according to character
	// update terminal

	// send char to output message queue if printable
	if((int)c >= 32 && (int)c <= 126){
		if (!(output_buf_idx >= OUTPUT_BUF_LEN)) {
			output_buf[output_buf_idx] = c;
			output_buf_idx++;
			//UART_DRV_SendDataBlocking(myUART_IDX, &c, sizeof(c), 1000);
		}
	}
	else{
		switch((int)c){
		case CTRLH:
			// clear current char
			if (!(output_buf_idx == 0)) { // if idx is zero, nothing in buffer
				deleteCharFromBuffer(output_buf_idx);
				output_buf_idx--;
			}
			break;
		case CTRLW:
			if (output_buf[output_buf_idx - 1] == 32) { // space
				// delete the space
				//UART_DRV_SendDataBlocking(myUART_IDX, "\b \b", 3, 1000); // delete the current character from output
				output_buf[output_buf_idx - 1] = '\0';
				if (output_buf_idx > 0) { // if zero, buffer is empty
					output_buf_idx--;
				}
				break;
			}
			for( output_buf_idx; (output_buf_idx > 0 && output_buf[output_buf_idx - 1] != 32); output_buf_idx--) {
				//delete chars until we hit space or end of buffer
				deleteCharFromBuffer(output_buf_idx);
			}
			break;
		case CTRLU:
			for( output_buf_idx; output_buf_idx > 0; output_buf_idx--) {
				//delete chars until end of buffer
				deleteCharFromBuffer(output_buf_idx);
			}
			break;
	    default:
	    	break;
	    }
	}
}

void deleteCharFromBuffer(int curr_buff_idx) {
	//UART_DRV_SendDataBlocking(myUART_IDX, "\b \b", 3, 1000); // delete the current character from output
	output_buf[output_buf_idx - 1] = '\0';
}
