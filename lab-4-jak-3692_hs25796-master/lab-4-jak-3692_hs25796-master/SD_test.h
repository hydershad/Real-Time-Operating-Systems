//SD_test.h
struct file_blocks{
	long address;
	struct file_blocks *next;
	
};
typedef struct file_blocks block_list;

struct file{
	
	char filename[20];
	struct file_blocks next_block;
};

typedef struct file fileType;

int block_read_speed(int num_blocks);
int block_write_speed(int num_blocks);
