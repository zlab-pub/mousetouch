#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <complex>
#include <iostream>
#include <valarray>
#include <fstream>
#include <vector>
#include <cstdlib>

#define BLOCK_SIZE 32
#define GLOBAL_RANGE 258
#define STEP 3
#define COUNT 636056

int RGB_arr[(GLOBAL_RANGE / STEP) * (GLOBAL_RANGE / STEP) * (GLOBAL_RANGE / STEP)][3];
int current_index;

bool REPETITION = true;
int repetition_time = 0;
// 0: no shift;
// 1: shift to right by half of block;
// 2: shift to down by half of block

struct bit_ele {
	char bit;
	// struct bit_ele *former;
	struct bit_ele *next;
	void* first_bit;
	bool last_bit;
};

struct bit_map {
	struct bit_ele* ele_arr[BLOCK_SIZE][BLOCK_SIZE];
	// int curr_index;
};


std::vector<double> read_filtered_data(char* file_path) {
	std::ifstream ifile(file_path, std::ios::in);
	std::vector<double> scores;

	//check to see that the file was opened correctly:
	if (!ifile.is_open()) {
		std::cerr << "There was a problem opening the input file!\n";
		exit(1);//exit or do additional error checking
	}

	double num = 0.0;
	//keep storing values from the text file so long as data exists:
	while (ifile >> num) {
		scores.push_back(num);
	}
	return scores;
}


struct bit_ele* read_swap_data(char* file_path) { // output: the first bit
	FILE *f;
	char char_buff;
	f = fopen(file_path, "r");
	if (f == NULL) {
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

///////////////////////////////////////////////////////////
// Below functions are used for multiple values per frame.
///////////////////////////////////////////////////////////

struct bit_map* read_location_data(char* file_path) {
	FILE *f;
	char char_buff;
	f = fopen(file_path, "r");
	if (f == NULL) {
		return NULL;
	}
	struct bit_map* big_map;
	big_map = (struct bit_map*) malloc(sizeof(struct bit_map));
	// big_map->curr_index = 0;
	struct bit_ele* curr_bit;
	int line_num = 0;
	char_buff = fgetc(f);
	if (char_buff != EOF) {
		curr_bit = (struct bit_ele*) malloc(sizeof(struct bit_ele));
		curr_bit->bit = char_buff;
		curr_bit->first_bit = curr_bit;
		big_map->ele_arr[(int)floor(line_num / BLOCK_SIZE)][(int)(line_num - BLOCK_SIZE * floor(line_num / BLOCK_SIZE))] \
			= curr_bit;
		line_num++;
		// curr_bit->former = NULL;
	}
	else return NULL;
	char_buff = fgetc(f);
	while (char_buff != EOF) {
		struct bit_ele* next_bit = (struct bit_ele*) malloc(sizeof(struct bit_ele));
		curr_bit->next = next_bit;
		curr_bit->last_bit = false;
		// next_bit->former = curr_bit;
		next_bit->first_bit = curr_bit->first_bit;
		next_bit->bit = char_buff;
		curr_bit = curr_bit->next;
		char_buff = fgetc(f);
		if (char_buff == '\n') {
			char_buff = fgetc(f);
			if (char_buff == EOF) break;
			curr_bit->next = NULL;
			curr_bit->last_bit = true;
			////////////
			curr_bit = (struct bit_ele*) malloc(sizeof(struct bit_ele));
			curr_bit->bit = char_buff;
			curr_bit->first_bit = curr_bit;
			big_map->ele_arr[(int)floor(line_num / BLOCK_SIZE)][(int)(line_num - BLOCK_SIZE * floor(line_num / BLOCK_SIZE))] = curr_bit;
			char_buff = fgetc(f);
			line_num++;
		}
	}
	curr_bit->next = NULL;
	curr_bit->last_bit = true;
	return big_map;

}

void free_location_data(struct bit_map* big_map) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			free_bit_arr((struct bit_ele*) big_map->ele_arr[i][j]->first_bit);
		}
	}
	free(big_map);
}

int normalized_block_index(int raw) {
	if (raw < 0) return 0;

	if (raw >= BLOCK_SIZE) return BLOCK_SIZE - 1;

	return raw;
}

/***************************************
@x, @y: indexes of pixel
@bit_eles: location encoding data
@width, @height: size of monitor
****************************************/
char get_bit_by_pixel(int x, int y, struct bit_map* big_map, \
	int width, int height) {
	int bound = width > height ? height : width; // assume always height
	if ((x > (width - bound) / 2 + bound) || (x < (width - bound) / 2)) return '1';

	int x_new, y_new;
	int pixels_per_block = bound / BLOCK_SIZE;
	///////////////////shift
	if (repetition_time == 0) {
		x_new = x;
		y_new = y;
	}
	else if (repetition_time == 1) {
		x_new = x + (int)pixels_per_block / 2;
		y_new = y;
	}
	else if (repetition_time == 2) {
		x_new = x;
		y_new = y + (int)pixels_per_block / 2;
	}
	else {
		x_new = x + (int)pixels_per_block / 2;
		y_new = y + (int)pixels_per_block / 2;
	}
	///////////////////shift

	int block_x = floor((x_new - ((width - bound) / 2)) / pixels_per_block);
	int block_y = floor(y_new / pixels_per_block);
	block_x = normalized_block_index(block_x);
	block_y = normalized_block_index(block_y);
	return big_map->ele_arr[block_x][block_y]->bit;
}


void move_next(struct bit_map* big_map) {
	bool go_first = false;
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			if (!big_map->ele_arr[i][j]->last_bit) {
				big_map->ele_arr[i][j] = big_map->ele_arr[i][j]->next;
			}
			else {
				big_map->ele_arr[i][j] = (struct bit_ele*) big_map->ele_arr[i][j]->first_bit;
				if (!go_first) go_first = true;
			}
		}
	}
	if (go_first) {
		if (repetition_time == 0) {
			repetition_time = 1;
			printf("Shift horizentally...\n");
		}
		else if (repetition_time == 1) {
			repetition_time = 2;
			printf("Shift vertically...\n");
		}
		else if (repetition_time == 2) {
			repetition_time = 3;
			printf("Shift horizentally again...\n");
		}
		else {
			repetition_time = 0;
			printf("Restore original Location\n");
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//////////// Below functions are used for Chromatic Transformation /////////////
////////////  Refer to http://www.easyrgb.com/en/math.php#text2/   /////////////
////////////////////////////////////////////////////////////////////////////////

struct CIE_val {
	float x;
	float y;
	float X;
	float Y;
	float Z;
};

struct RGB_val {
	unsigned int R;
	unsigned int G;
	unsigned int B;
	float r;
	float g;
	float b;
};

struct RGB_val init_RGB(unsigned int R, unsigned int G, unsigned int B) {
	struct RGB_val rgb_v;
	rgb_v.R = R;
	rgb_v.G = G;
	rgb_v.B = B;
	rgb_v.r = (float)R / 255.0f;
	rgb_v.g = (float)G / 255.0f;
	rgb_v.b = (float)B / 255.0f;
	return rgb_v;
}

struct CIE_val init_CIE(float X, float Y, float Z) {
	struct CIE_val cie_v;
	cie_v.X = X;
	cie_v.Y = Y;
	cie_v.Z = Z;
	cie_v.x = X / (X + Y + Z);
	cie_v.y = Y / (X + Y + Z);
	return cie_v;
}

struct CIE_val init_CIE_directly(float x, float y, float Y) {
	float X = x * (Y / y);
	float Z = (1 - x - y) * (Y / y);
	struct CIE_val cie_v;
	cie_v.X = X;
	cie_v.Y = Y;
	cie_v.Z = Z;
	cie_v.x = x;
	cie_v.y = y;
	return cie_v;
}

struct CIE_val RGB_2_CIE(struct RGB_val rgb_v) {
	float var_R = rgb_v.r;
	float var_G = rgb_v.g;
	float var_B = rgb_v.b;
	float X, Y, Z;
	if (var_R > 0.04045) var_R = pow(((var_R + 0.055) / 1.055), 2.4);
	else var_R = var_R / 12.92;
	if (var_G > 0.04045) var_G = pow(((var_G + 0.055) / 1.055), 2.4);
	else var_G = var_G / 12.92;
	if (var_B > 0.04045) var_B = pow(((var_B + 0.055) / 1.055), 2.4);
	else var_B = var_B / 12.92;

	var_R = var_R * 100.0f;
	var_G = var_G * 100.0f;
	var_B = var_B * 100.0f;

	X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
	Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
	Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;
	return init_CIE(X, Y, Z);
}

struct RGB_val CIE_2_RGB(struct CIE_val cie_v) {
	unsigned int sR, sG, sB;
	float var_X = cie_v.X / 100.0f;
	float var_Y = cie_v.Y / 100.0f;
	float var_Z = cie_v.Z / 100.0f;

	float var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
	float var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
	float var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

	if (var_R > 0.0031308) var_R = 1.055 * (pow(var_R, (1.0f / 2.4))) - 0.055;
	else                     var_R = 12.92 * var_R;
	if (var_G > 0.0031308) var_G = 1.055 * (pow(var_G, (1.0f / 2.4))) - 0.055;
	else                     var_G = 12.92 * var_G;
	if (var_B > 0.0031308) var_B = 1.055 * (pow(var_B, (1.0f / 2.4))) - 0.055;
	else                     var_B = 12.92 * var_B;

	sR = var_R * 255;
	sG = var_G * 255;
	sB = var_B * 255;
	return init_RGB(sR, sG, sB);
}

float GB_line(float x) {
	return 24.15 * x - 3.562;
}

float GR_line(float x) {
	return -0.8251 * x + 0.8580;
}

float BR_line(float x) {
	return 0.5510 * x - 0.02265;
}

bool check_valid(float x, float y) {
	//printf("%f, %f\n", x, y);
	float y_GB = GB_line(x);
	float y_GR = GR_line(x);
	float y_BR = BR_line(x);
	// printf("%f, %f, %f\n", y_GB, y_GR, y_BR);
	if (y_GB >= y && y_GR >= y && y_BR <= y) return true;
	return false;
}

void pick_with_alpha(struct RGB_val* colors, struct RGB_val origial_val, float alpha) {
	struct CIE_val cie_v = RGB_2_CIE(origial_val);
	// float slope = rand();
	// slope = rand() % 2 == 0 ? slope : -slope;
	float x1, x2, y1, y2;
	srand(time(NULL));
	do {
		x1 = (float)rand() / (float)RAND_MAX * (0.64 - 0.15); // select random x
		// x2 = (x1 - alpha * x1) / (1 - alpha);
		x2 = (cie_v.x - alpha * x1) / (1 - alpha);

		y1 = (float)rand() / (float)RAND_MAX * (0.712 - 0.06);
		// y2 = (y1 - alpha * y1) / (1 - alpha);
		y2 = (cie_v.y - alpha * y1) / (1 - alpha);
	} while (!check_valid(x1, y1) || !check_valid(x2, y2));
	float Y2 = 2 * (1 - alpha) * cie_v.Y;
	float Y1 = Y2 * (alpha / (1 - alpha));
	printf("CIE(x,y,Y): %f, %f, %f\n", x1, y1, Y1);
	printf("CIE(x,y,Y): %f, %f, %f\n", x2, y2, Y2);
	colors[0] = CIE_2_RGB(init_CIE_directly(x1, y1, Y1));
	colors[1] = CIE_2_RGB(init_CIE_directly(x2, y2, Y2));
	if (colors[0].R + colors[0].G + colors[0].B < colors[1].R + colors[1].G + colors[1].B) {
		struct RGB_val temp = init_RGB(colors[0].R, colors[0].G, colors[0].B);
		colors[0] = colors[1];
		colors[1] = temp;
	}
	assert(colors[0].R + colors[0].G + colors[0].B >= colors[1].R + colors[1].G + colors[1].B);
}

void print_RGB(struct RGB_val rgb_v) {
	printf("rgb: (%f, %f, %f)\n", rgb_v.r, rgb_v.g, rgb_v.b);
	printf("RGB: (%u, %u, %u)\n", rgb_v.R, rgb_v.G, rgb_v.B);
}

void print_CIE(struct CIE_val cie_v) {
	printf("CIE(x, y, Y): (%f, %f, %f)\n", cie_v.x, cie_v.y, cie_v.Y);
	printf("CIE(X, Y, Z): (%f, %f, %f)\n", cie_v.X, cie_v.Y, cie_v.Z);
}


//////////////////////////////////////////
/////////For Chromatic Test///////////////
//////////////////////////////////////////
#ifdef __linux__ 
void permutation(int* range1, int* range2, int* range3, int size1, int size2, int size3) {
	int arr_len1 = size1;
	int arr_len2 = size2;
	int arr_len3 = size3;

	int ele_index_not_1;
	if (arr_len1 != 1) ele_index_not_1 = 1;
	else if (arr_len2 != 1) ele_index_not_1 = 2;
	else if (arr_len3 != 1) ele_index_not_1 = 3;
	else ele_index_not_1 = 0;

	if (ele_index_not_1 == 1) {
		int temp_range[1];
		for (int i = 0; i < arr_len1; i++) {
			temp_range[0] = range1[i];
			permutation(temp_range, range2, range3, 1, size2, size3);
		}
	}
	else if (ele_index_not_1 == 2) {
		int temp_range[1];
		for (int i = 0; i < arr_len2; i++) {
			temp_range[0] = range2[i];
			permutation(range1, temp_range, range3, 1, 1, size3);
		}
	}
	else if (ele_index_not_1 == 3) {
		int temp_range[1];
		for (int i = 0; i < arr_len3; i++) {
			temp_range[0] = range3[i];
			permutation(range1, range2, temp_range, 1, 1, 1);
		}
	}
	// Base case
	else if (ele_index_not_1 == 0) {
		RGB_arr[current_index][0] = range1[0];
		RGB_arr[current_index][1] = range2[0];
		RGB_arr[current_index][2] = range3[0];
		current_index++;
	}
	else printf("Unknown\n");
}

void permutation_init(int step) {
	assert(GLOBAL_RANGE % step == 0);
	int range[GLOBAL_RANGE / step];
	for (int i = 0; i < GLOBAL_RANGE / step; i++) {
		range[i] = i * step;
	}
	// printf("Initial len: %d\n", GLOBAL_RANGE / step);
	int size = GLOBAL_RANGE / step;
	permutation(range, range, range, size, size, size);
}

#endif




