typedef struct {
    const char* font_file;
    int font_size;
	Color PRIMARY_COLOR;
	Color SECONDARY_COLOR;
	Color TERTIARY_COLOR;
} Theme;

Theme simple = {
    .font_file = "default_font.ttf", // Assuming a default font file
    .font_size = 30,
    .PRIMARY_COLOR = COLOR_WHITE,
    .SECONDARY_COLOR = COLOR_BLACK,
    .TERTIARY_COLOR = COLOR_GRAY // Assuming 'gray' refers to TERTIARY_COLOR
};

/*
	USAGE:
	you can pass simple.PRIMARY_COLOR or anyother arguement to functions'
*/

// TODO : add more themes