//------------------------- APP ___________
typedef struct {
    Parent* window; // Window struct from window.h
} App;

App init(void) {
    App app = {0}; // Initialize struct members to zero
    // Initialize fonts or other resources here if needed
    //   app.theme = ;
    return app;
}

void app_run_(Parent* parent) {
    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else {
                update_all_registered_containers(event);
                update_all_registered_drops(event);
                update_all_registered_radios(event);
                update_all_registered_textboxs(event);
                update_all_registered_sliders(event);  // Update sliders
            }
        }

        clear_screen_(&parent->base, COLOR_GREEN);
        render_all_registered_containers();
        render_all_registered_drops();
        render_all_registered_radios();
        render_all_registered_textboxs();
        render_all_registered_sliders();  // Render sliders
        present_(&parent->base);
    }

    destroy_parent(parent);
}