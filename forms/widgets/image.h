#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<stdlib.h>
typedef struct{
	Parent *parent;
	int x, y, w, h;
	const char * file_path;
	SDL_Texture *texture;
}Image;

void register_widget_image(Image * image); // function prototype

Image * new_image_(Parent * parent, int x, int y, const char * file_path, int w, int h  ){
	if(!parent || !parent->base.sdl_renderer){
		printf("Invalid parent or renderer for image widget");
		return NULL;
	}
	
	Image *new_image = (Image*)malloc(sizeof(Image));
	
	if(!new_image){
		printf("failed to allocate memory for image");
		return NULL;
	}
	new_image->parent = parent;
	new_image->x=x;
	new_image->y=y;
	new_image->file_path=file_path;
	new_image->w=w;
	new_image->h=h;
	new_image->texture= IMG_LoadTexture(parent->base.sdl_renderer, file_path);
	if(!new_image->texture){
		printf("Failed to load img %s : %s\n", file_path, IMG_GetError());
		free(new_image);
		return NULL;
	}
	
	register_widget_image(new_image);
	return new_image;
}

void render_image(Image * image){
	if (!image || !image->parent || !image->parent->base.sdl_renderer || !image->parent->is_open) {
        printf("Invalid image widget, renderer, or parent is not open\n");
        return;
    }
        
     // Calculate absolute position relative to parent
    int abs_x = image->x + image->parent->x;
    int abs_y = image->y + image->parent->y + image->parent->title_height;
    
	draw_image_from_texture_(&(image->parent->base), image->texture, abs_x, abs_y, image->w, image->h);
}

void update_image(Image *image, SDL_Event event){} // incase in the future, image needs to be resizeable, movable, ...

void free_image(Image *image) {
    if (image) {
        if (image->texture) {
        	SDL_DestroyTexture(image->texture);
        	image->texture=NULL;
        }
        free(image);
    }
}


// Registration of widgets for rendering
#define MAX_IMAGES 100
Image * image_widgets[MAX_IMAGES];

int images_count =0;

void register_widget_image(Image* image) {
    if (images_count < MAX_IMAGES) {
        image_widgets[images_count] = image;
        images_count++;
    }
}

void render_all_registered_images(void) {
    for (int i = 0; i < images_count; i++) {
        if (image_widgets[i]) {
            render_image(image_widgets[i]);
        }
    }
}

void update_all_registered_images(SDL_Event event) {
    for (int i = 0; i < images_count; i++) {
        if (image_widgets[i]) {
            update_image(image_widgets[i], event);
        }
    }
}

void free_all_registered_images(void){
	for(int i=0; i< images_count; i++){
		free_image(image_widgets[i]);
		image_widgets[i]=NULL;
	}
	images_count = 0;
}