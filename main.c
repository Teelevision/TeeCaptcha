#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <string.h>

/* errors */
#define ERR_PARAM 1
#define ERR_CREATE_CAPTCHA 2
#define ERR_LOAD_DATA 3

/* pixel */
typedef struct {
   png_byte red;
   png_byte green;
   png_byte blue;
   png_byte alpha;
} pixel_t;

/* ressources */
typedef struct {
	pixel_t **rows;
} ressource_t;
ressource_t *doodads;
ressource_t *tees;

/* tees */
typedef struct {
	float weyepos;
	float heyepos;
	int eyes;
	ressource_t *res;
} tee_t;
int num_tees = 0;

/* doodads */
typedef struct {
	ressource_t *res;
} doodad_t;
int num_doodads = 0;

/* fields */
enum field_types_t {
	field_type_tee = 0,
	field_type_doodad,
	num_field_types
};
typedef struct {
	enum field_types_t type;
	tee_t *tee;
	doodad_t *doodad;
	float wpos;
	float hpos;
} field_t;

/* captcha */
typedef struct {
	int width;
	int height;
	int size;
	int target;
	field_t *fields;
	pixel_t **rows;
	int width_px;
	int height_px;
	int size_px;
} captcha_t;

/* settings */
typedef struct {
	/* path of the output file */
	char *out;
	/* how many fields in the width and height */
	int fields_w;
	int fields_h;
	/* Set these >1 to have randomly larger captchas, e.g. fields_w = 5, fields_w_range = 3, then the width can be 5-7. Set this <=1 to disable this feature. */
	int fields_w_range;
	int fields_h_range;
	/* there have to be more fields in the height than width */
	int fields_h_gt_w;
	/* width and height of the fields */
	int field_size;
	/* how many pixels to crop off the edges of the tee's body */
	int tee_offset;
} settings;

/* default settings */
settings s = {
	"captcha.png",
	4, 6,
	1, 1,
	0,
	56,
	4
};

/* working directory */
char *wd;

int save_bitmap(char* filename, pixel_t **rows, int width, int height) {
	int code = 0;
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	
	/* open file */
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename);
		code = 1;
		goto fail;
	}
	
	/* init write structure */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate write struct\n");
		code = 2;
		goto fail;
	}
	
	/* init info structure */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		code = 3;
		goto fail;
	}
	
	/* setup exception handling */
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creaton\n");
		code = 4;
		goto fail;
	}
	
	png_init_io(png_ptr, fp);
	
	/* write header */
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	/* write info */
	png_write_info(png_ptr, info_ptr);
	
	/* write data */
	int y;
	for (y = 0; y < height; y++) {
		png_write_row(png_ptr, (png_bytep)(rows[y]));
	}
	png_write_end(png_ptr, NULL);
	
	/* tidy up */
	fail:
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	
	return code;
}

int load_data(char *filename, png_bytep **rows, int *width, int *height, png_byte *color_type, png_byte *bit_depth) {
	int code = 0;
	
	/* open file for reading */
	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s for reading\n", filename);
		code = 1;
		goto fail;
	}
	
	/* check PNG header */
	char header[8];
	fread(header, 1, 8, fp);
	if (!png_check_sig(header, 8)) {
		fprintf(stderr, "File %s is not a PNG\n", filename);
		code = 2;
		goto fail;
	}
	
	/* init reading */
	png_structp png_ptr;
	png_infop info_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate read struct\n");
		code = 3;
		goto fail;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		code = 4;
		goto fail;
	}
	
	/* setup exception handling */
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during initialization of reading\n");
		code = 5;
		goto fail;
	}
	
	/* prepare reading */
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	
	/* read info */
	png_read_info(png_ptr, info_ptr);
	*width = png_get_image_width(png_ptr, info_ptr);
	*height = png_get_image_height(png_ptr, info_ptr);
	*color_type = png_get_color_type(png_ptr, info_ptr);
	*bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	
	/* setup exception handling */
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during reading file %s\n", filename);
		code = 6;
		goto fail;
	}
	
	/* prepare memory */
	*rows = (png_bytep*)malloc(sizeof(png_bytep) * *height);
	int y;
	for (y = 0; y < *height; y++) {
		(*rows)[y] = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr));
	}
	
	/* read actual data */
	png_read_image(png_ptr, *rows);
	
	/* tidy up */
	fail:
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
	
	return code;
}

int load_tees() {
	
	/* init file path */
	char *filename = (char*)malloc(strlen(wd) + 9);
	strcpy(filename, wd);
	strcat(filename, "tees.png");
	
	int width, height;
	png_byte color_type, bit_depth;
	pixel_t **rows;
	
	/* load PNG file */
	if (load_data(filename, (png_bytep**)&rows, &width, &height, &color_type, &bit_depth)) {
		return 1;
	}
	
	/* check dimensions */
	if (width != 128 || height % 64 > 0 || bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGBA) {
		fprintf(stderr, "Image tees.png has not the right dimensions (width: 128, height: 64/tee, 8 bit/channel RGBA)\n");
		return 2;
	}
	num_tees = height / 64;
	
	/* link tees */
	tees = (ressource_t*)malloc(sizeof(ressource_t) * num_tees);
	int y;
	for (y = 0; y < num_tees; y++) {
		tees[y].rows = &(rows[y*64]);
	}
	
	return 0;
}

int load_doodads() {
	
	/* init file path */
	char *filename = (char*)malloc(strlen(wd) + 12);
	strcpy(filename, wd);
	strcat(filename, "doodads.png");
	
	int width, height;
	png_byte color_type, bit_depth;
	pixel_t **rows;
	
	/* load PNG file */
	if (load_data(filename, (png_bytep**)&rows, &width, &height, &color_type, &bit_depth)) {
		return 1;
	}
	
	/* check dimensions */
	if (width != 48 || height % 48 > 0 || bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGBA) {
		fprintf(stderr, "Image %s has not the right dimensions (width: 48, height: 48/doodad, 8 bit/channel RGBA)\n", filename);
		return 2;
	}
	num_doodads = height / 48;
	
	/* link doodads */
	doodads = (ressource_t*)malloc(sizeof(ressource_t) * num_doodads);
	int y;
	for (y = 0; y < num_doodads; y++) {
		doodads[y].rows = &(rows[y*48]);
	}
	
	return 0;
}

void copy_rect(pixel_t **dest, pixel_t **src, int dx, int dy, int sx, int sy, int w, int h) {
	int x, y;
	pixel_t *spx, *dpx;
	float alpha;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			spx = &(src[sy+y][sx+x]);
			dpx = &(dest[dy+y][dx+x]);
			if (spx->alpha) {
				alpha = spx->alpha / 255.0;
				dpx->red = (int)(alpha * spx->red + (1 - alpha) * dpx->red);
				dpx->green = (int)(alpha * spx->green + (1 - alpha) * dpx->green);
				dpx->blue = (int)(alpha * spx->blue + (1 - alpha) * dpx->blue);
				dpx->alpha = dpx->alpha + alpha * (255 - dpx->alpha);
			}
		}
	}
}

int create_captcha(char *filename) {
	int i, x, y;
	
	/* create captcha */
	captcha_t c;
	
	/* fields */
	/* width */
	if (s.fields_w_range > 1) {
		c.width = s.fields_w + rand() % s.fields_w_range;
	} else {
		c.width = s.fields_w;
	}
	/* height */
	if (s.fields_h_range > 1) {
		if (s.fields_h_gt_w && c.width >= s.fields_h) {
			c.height = c.width + 1 + rand() % (s.fields_h + s.fields_h_range - c.width - 1);
		} else {
			c.height = s.fields_h + rand() % s.fields_h_range;
		}
	} else {
		c.height = s.fields_h;
	}
	/* size */
	c.size = c.width * c.height;
	
	
	/* init fields */
	c.fields = (field_t*)malloc(sizeof(field_t) * c.size);
	int *teepos = (int*)malloc(sizeof(int) * c.size);
	int numtees = 0;
	/* fill fields */
	for (i = 0; i < c.size; i++) {
		field_t *f = &(c.fields[i]);
		
		/* type */
		/* 75% chance for a tee or at least size/2+1 many tees */
		f->type = (rand() % 4 || (c.size - i + numtees) <= (c.size / 2 + 1)) ? field_type_tee : field_type_doodad;
		switch (f->type) {
			
			/* tee */
			case field_type_tee:
				f->tee = (tee_t*)malloc(sizeof(tee_t));
				f->tee->res = &(tees[rand() % num_tees]);
				f->tee->weyepos = (50 + (rand() % 51)) / (rand() % 2 ? 100.0 : -100.0);
				f->tee->heyepos = (50 + (rand() % 51)) / (rand() % 2 ? 100.0 : -100.0);
				f->tee->eyes = rand() % 2 ? rand() % 6 : 0;
				teepos[numtees++] = i;
				break;
			
			/* doodad */
			case field_type_doodad:
				f->doodad = (doodad_t*)malloc(sizeof(doodad_t));
				f->doodad->res = &(doodads[rand() % num_doodads]);
				break;
				
		}
		
		/* position */
		f->wpos = (rand() % 100) / (rand() % 2 ? 100.0 : -100.0);
		f->hpos = (rand() % 100) / (rand() % 2 ? 100.0 : -100.0);
		
	}
	
	/* select target tee */
	c.target = teepos[rand() % numtees];
	
	/* calculate image format */
	c.width_px = s.field_size * c.width;
	c.height_px = s.field_size * c.height;
	c.size_px = c.width_px * c.height_px;
	
	/* init image */
	c.rows = (pixel_t**)malloc(c.height_px * sizeof(pixel_t*));
	for (y = 0; y < c.height_px; y++) {
		c.rows[y] = (pixel_t*)malloc(c.width_px * sizeof(pixel_t));
	}
	
	/* background */
	pixel_t background = { 161, 110, 54, 255 };
	for (x = 0; x < c.width_px; x++) {
		memcpy(&(c.rows[0][x]), &background, sizeof(pixel_t));
	}
	for (y = 0; y < c.height_px; y++) {
		memcpy(c.rows[y], c.rows[0], sizeof(pixel_t) * c.width_px);
	}
	
	/* insert tees and doodads */
	int h, w;
	for (h = 0; h < c.height; h++) {
		for (w = 0; w < c.width; w++) {
			i = h * c.width + w;
			field_t *f = &(c.fields[i]);
			
			/* tee */
			if (f->type == field_type_tee) {
				pixel_t **frows = f->tee->res->rows;
				
				/* general position */
				int tee_width = 48 - 2 * s.tee_offset;
				int margin = (s.field_size - tee_width) / 2;
				int wpos = w * s.field_size + margin + f->wpos * margin;
				int hpos = h * s.field_size + margin + f->hpos * margin;
				int weyepos = f->tee->weyepos * 6;
				int heyepos = f->tee->heyepos * 4;
				
				/* foot 1 (behind body) */
				copy_rect(c.rows, frows, wpos + tee_width / 2 - 16, hpos + tee_width / 2 + 4, 104, 32, 16, 16);
				copy_rect(c.rows, frows, wpos + tee_width / 2 - 16, hpos + tee_width / 2 + 4, 104, 16, 16, 16);
				
				/* body */
				copy_rect(c.rows, frows, wpos, hpos, 48 + s.tee_offset, s.tee_offset, tee_width, tee_width);
				copy_rect(c.rows, frows, wpos, hpos, s.tee_offset, s.tee_offset, tee_width, tee_width);
				
				/* eyes */
				if (c.target != i) {
					int weyes = 32 + f->tee->eyes * 16;
					int eyew = wpos + tee_width / 2 - 8 + weyepos;
					int eyeh = hpos + tee_width / 2 - 8 - 2 + heyepos;
					copy_rect(c.rows, frows, eyew - 3, eyeh, weyes, 48, 16, 16);
					copy_rect(c.rows, frows, eyew + 3, eyeh, weyes, 48, 16, 16);
				}
				
				/* foot 2 (front) */
				copy_rect(c.rows, frows, wpos + tee_width / 2, hpos + tee_width / 2 + 4, 104, 32, 16, 16);
				copy_rect(c.rows, frows, wpos + tee_width / 2, hpos + tee_width / 2 + 4, 104, 16, 16, 16);
			}
			
			/* doodad */
			else {
				// fprintf(stderr, "%d %d\n", h, w);
				int margin = (s.field_size - 48) / 2;
				int wpos = w * s.field_size + margin + f->wpos * margin;
				int hpos = h * s.field_size + margin + f->hpos * margin;
				copy_rect(c.rows, f->doodad->res->rows, wpos, hpos, 0, 0, 48, 48);
			}
			
		}
	}
	
	/* write captcha PNG file */
	if (save_bitmap(filename, c.rows, c.width_px, c.height_px) > 0) {
		return 1;
	}
	
	/* output dimensions and position of the tee to find */
	printf("%d %d %d\n", c.width, c.height, c.target);
	return 0;
}

int main (int argc, char *argv[]) {
	int i, x, y;
	
	/* read settings */
	for (i = 1; i < argc; i++) {
		
		/* help */
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf("TeeCaptcha creates Teeworlds PNG captchas.\n", argv[0]);
			printf("Usage: %s FILES\n", argv[0]);
			printf("Options:\n");
			printf("\nBasic options:\n");
			printf(" -x WIDTH     Number of fields in the width (default: 4)\n");
			printf(" -y HEIGHT    Number of fields in the height (default: 6)\n");
			printf(" -X RANGEX    Range of fields in the width (default: 0)\n");
			printf(" -Y RANGEY    Range of fields in the height (default: 0)\n");
			printf("\nFurther options:\n");
			printf(" -g           HEIGHT > WIDTH\n");
			printf(" -G           Invert -g (default)\n");
			printf(" -s SIZE      Size of a field in pixels (default: 56)\n");
			printf(" -O OFFSET    Skin offset (default: 4)\n");
			printf(" -h --help    Display this help information\n");
			printf("\nExamples:\n");
			printf(" %s -x 4 -y 6 c.png\n", argv[0]);
			printf("    Creates c.png with 4x6 fields.\n");
			printf(" %s -x 4 -y 6 -X 4 -Y 4 c.png\n", argv[0]);
			printf("    Creates c.png with min 4x6 to max 7x9 fields.\n");
			printf(" %s -x 4 -y 6 -X 4 -Y 4 -g c.png\n", argv[0]);
			printf("    Same as above, but HEIGHT > WIDTH.\n");
			printf(" %s a.png b.png c.png\n", argv[0]);
			printf("    Creates 3 different files.\n");
			printf("\nPlease report bugs: marius@teele.eu\n");
			return 0;
		}
		
		/* width */
		else if (strcmp(argv[i], "-x") == 0) {
			if (++i < argc) {
				s.fields_w = atoi(argv[i]);
			} else {
				fprintf(stderr, "Argument -x is incomplete, WIDTH required!\n");
				return ERR_PARAM;
			}
		}
		
		/* height */
		else if (strcmp(argv[i], "-y") == 0) {
			if (++i < argc) {
				s.fields_h = atoi(argv[i]);
			} else {
				fprintf(stderr, "Argument -y is incomplete, HEIGHT required!\n");
				return ERR_PARAM;
			}
		}
		
		/* width range */
		else if (strcmp(argv[i], "-X") == 0) {
			if (++i < argc) {
				s.fields_w_range = atoi(argv[i]);
			} else {
				fprintf(stderr, "Argument -X is incomplete, RANGEX required!\n");
				return ERR_PARAM;
			}
		}
		
		/* height range */
		else if (strcmp(argv[i], "-Y") == 0) {
			if (++i < argc) {
				s.fields_h_range = atoi(argv[i]);
			} else {
				fprintf(stderr, "Argument -Y is incomplete, RANGEY required!\n");
				return ERR_PARAM;
			}
		}
		
		/* hight > width */
		else if (strcmp(argv[i], "-g") == 0) {
			s.fields_h_gt_w = 1;
		}
		else if (strcmp(argv[i], "-G") == 0) {
			s.fields_h_gt_w = 0;
		}
		
		/* field size */
		else if (strcmp(argv[i], "-s") == 0) {
			if (++i < argc) {
				s.field_size = atoi(argv[i]);
			} else {
				fprintf(stderr, "Argument -s is incomplete, SIZE required!\n");
				return ERR_PARAM;
			}
		}
		
		/* tee offset */
		else if (strcmp(argv[i], "-O") == 0) {
			if (++i < argc) {
				s.tee_offset = atoi(argv[i]);
			} else {
				fprintf(stderr, "Argument -O is incomplete, OFFSET required!\n");
				return ERR_PARAM;
			}
		}
		
		/* must be a file (see below) */
		else {
			break;
		}
	}
	
	/* check options */
	if (s.fields_w <= 0 || s.fields_w_range <= 0 ||s.fields_h <= 0 || s.fields_h_range <= 0) {
		fprintf(stderr, "Options -x, -X, -y, -Y must be >0!\n");
		return ERR_PARAM;
	}
	if (s.field_size < 48) {
		fprintf(stderr, "Option -s SIZE must be >=48!\n");
		return ERR_PARAM;
	}
	if (s.tee_offset >= 24) {
		fprintf(stderr, "Option -O OFFSET must be <24!\n");
		return ERR_PARAM;
	}
	if (s.fields_h_gt_w == 1 && ((s.fields_w + s.fields_w_range) >= (s.fields_h + s.fields_h_range))) {
		fprintf(stderr, "Dimensions are not working with -g option, increase y/Y or decrease x/X!\n");
		return ERR_PARAM;
	}
	
	/* file parameters */
	if ((argc - i) <= 0) {
		fprintf(stderr, "Missing FILES!\n");
		return ERR_PARAM;
	}
	char **files = (char**)malloc(sizeof(char*) * (argc - i));
	int numfiles = 0;
	for (; i < argc; i++) {
		files[numfiles++] = argv[i];
	}
	
	/* get the program's directory */
	wd = strcat(dirname(argv[0]), "/");
	
	/* init randomness */
	FILE *fp;
	unsigned int seed;
	fp = fopen("/dev/urandom", "r");
	fread(&seed, sizeof(unsigned int), 1, fp);
	fclose(fp);
	srand(seed);
	
	/* load data */
	if (load_tees() || load_doodads()) {
		fprintf(stderr, "Error loading data!\n");
		return ERR_LOAD_DATA;
	}
	
	/* create captchas */
	for (i = 0; i < numfiles; i++) {
		if (create_captcha(files[i])) {
			fprintf(stderr, "Error creating captcha!\n");
			return ERR_CREATE_CAPTCHA;
		}
	}
	
	return 0;
}
