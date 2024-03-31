#include <pipe.h>
#include <signal.h>
#include <process.h>
#include <scheduler.h>

extern process_t *current_process;

void wake_up(process_t **p);
void goto_sleep(process_t **p);

i32 read_pipe(struct ext2_inode *inode, i8 *buf, u32 count) {
	i8 *b = buf;
	
	while (PIPE_EMPTY(inode)) {
		wake_up(&inode->i_wait);
		if (inode->i_count != 2) {
			return 0;
		}
		goto_sleep(&inode->i_wait);
	}
	while (count > 0 && !(PIPE_EMPTY(inode))) {
		--count;
		*b = ((i8 *)inode->i_block[0])[PIPE_TAIL(inode)];
		++b;
		++PIPE_TAIL(inode);
		PIPE_TAIL(inode) &= (4096 - 1);
	}
	return b - buf;
}

i32 write_pipe(struct ext2_inode *inode, i8 *buf, u32 count) {
	i8 *b = buf;

	wake_up(&inode->i_wait);
	if (inode->i_count != 2) {
		sigaddset(&current_process->sigpending, SIGPIPE);
		return -1;
	}
	while (count-- > 0) {
		while (PIPE_FULL(inode)) {
			wake_up(&inode->i_wait);
			if (inode->i_count != 2) {
				sigaddset(&current_process->sigpending, SIGPIPE);
				return -1;
			}
			goto_sleep(&inode->i_wait);
		}
		((i8 *)inode->i_block[0])[PIPE_HEAD(inode)] = *b;
		++b;
		++PIPE_HEAD(inode);
		PIPE_HEAD(inode) &= (4096 - 1);
		wake_up(&inode->i_wait);
	}
	wake_up(&inode->i_wait);
	return b - buf;
}
