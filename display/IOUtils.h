#include <malloc.h>
struct bit_ele {
	char bit;
	// struct bit_ele *former;
	struct bit_ele *next;
	void* first_bit;
	bool last_bit;
};
struct bit_ele* read_swap_data(char* file_path) { // output: the first bit
	FILE *f;
	char char_buff;
	f = fopen(file_path, "r");
	if (f == NULL) {
		std::cout << "no file" <<std::endl;
		return NULL;
	}
	struct bit_ele* curr_bit;

	char_buff = fgetc(f);
	if (char_buff != EOF) {
		curr_bit = (struct bit_ele*) malloc(sizeof(struct bit_ele));
		curr_bit->bit = char_buff;
		curr_bit->first_bit = curr_bit;
		// curr_bit->former = NULL;
	}
	else return NULL;
	char_buff = fgetc(f);
	while (char_buff != EOF && char_buff != '\n') {
		struct bit_ele* next_bit = (struct bit_ele*) malloc(sizeof(struct bit_ele));
		curr_bit->next = next_bit;
		curr_bit->last_bit = false;
		// next_bit->former = curr_bit;
		next_bit->first_bit = curr_bit->first_bit;
		next_bit->bit = char_buff;
		curr_bit = curr_bit->next;
		char_buff = fgetc(f);
	}
	curr_bit->next = NULL;
	curr_bit->last_bit = true;
	return (struct bit_ele*) curr_bit->first_bit;
}


void free_bit_arr(struct bit_ele* barr) { // input: the first bit
	assert(barr == barr->first_bit);
	if (barr->last_bit) {
		free(barr);
		return;
	}
	struct bit_ele* temp;
	while (!barr->next->last_bit) {
		temp = barr;
		barr = barr->next;
		free(temp);
	}
	free(barr->next);
}
