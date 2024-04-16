#include <timer.h>
#include <isr.h>
#include <port.h>
#include <panic.h>
#include <process.h>
#include <scheduler.h>

extern process_t *current_process;

u32 startup_time;
u32 ticks = 0;

static void timer_handler(registers_state *regs) {
	(void)regs;
	++ticks;
	if (current_process == NULL) {
		return;
	}
	/* TODO: Think about stime and utime */
	if (regs->cs == 0x18 || regs->cs == 0x20) {
		++current_process->stime;
	} else {
		++current_process->utime;
	}
	if ((--current_process->timeslice) > 0) {
		return;
	}
	current_process->timeslice = 0;
	schedule();
}

u32 get_current_time() {
	return startup_time + ticks / TIMER_FREQ;
}

void timer_init(u32 freq) {
	u8 command_word, low_byte, high_byte; 
	u16 divisor;

	divisor = INPUT_FREQUENCY_IN_HZ / freq;
	low_byte = (u8)(divisor & 0xFF);
	high_byte = (u8)((divisor >> 8) & 0xFF);

	command_word = 0x36; /* 00110110b */
	/* Set binary counting, set mode 3(Square Wave Generator),
	 * read LSB first then MSB, select Counter 0 
	 */
	port_outb(CONTROL_WORD_REGISTER, command_word);	

	port_outb(COUNTER_0_REGISTER, low_byte);
	port_outb(COUNTER_0_REGISTER, high_byte);

	register_interrupt_handler(IRQ0, timer_handler);
	debug("Timer has been initialized\r\n");
}
