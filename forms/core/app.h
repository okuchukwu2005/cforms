//------------------------- APP ___________
typedef struct {
    Parent* window; // Window struct from window.h
} App;

App init(void) {
    App app = {0}; // Initialize struct members to zero
    
    // Set default theme (e.g., light mode)
    set_theme(&THEME_LIGHT);
    
    // Initialize other global resources if needed
    // For example: Load global fonts, set SDL hints, or init other subsystems
    // SDL_Init(SDL_INIT_EVERYTHING);  // If not already in new_window_
    // TTF_Init();  // If not already handled
    
    return app;
}

int is_any_text_widget_active(void) {
    // Check entries
    for (int i = 0; i < entrys_count; i++) {
        if (entry_widgets[i] && entry_widgets[i]->is_active) {
            return 1;
        }
    }
    // Check textboxes
    for (int i = 0; i < textboxs_count; i++) {
        if (textbox_widgets[i] && textbox_widgets[i]->is_active) {
            return 1;
        }
    }
    return 0;
}

void app_run_(Parent* parent) {
    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else {
                // New: Handle theme switching on key press
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_l:  // 'L' for Light
                            set_theme(&THEME_LIGHT);
                            printf("Switched to Light theme\n");
                            break;
                        case SDLK_d:  // 'D' for Dark
                            set_theme(&THEME_DARK);
                            printf("Switched to Dark theme\n");
                            break;
                        case SDLK_h:  // 'H' for Hacker
                            set_theme(&THEME_HACKER);
                            printf("Switched to Hacker theme\n");
                            break;
                        default:
                            break;
                    }
                }

                update_all_registered_containers(event);
                update_all_registered_drops(event);
                update_all_registered_radios(event);
                update_all_registered_entrys(event);
                update_all_registered_textboxs(event);
                update_all_registered_sliders(event);  // Update sliders
                update_all_registered_buttons(event);
                update_all_registered_texts(event);

                // Global text input management after all updates
                if (is_any_text_widget_active()) {
                    SDL_StartTextInput();
                } else {
                    SDL_StopTextInput();
                }
            }
        }

        clear_screen_(&parent->base, COLOR_GREEN);  // TODO: Use current_theme->bg_primary here for themed bg?
        render_all_registered_containers();
        render_all_registered_drops();
        render_all_registered_radios();
        render_all_registered_entrys();
        render_all_registered_textboxs();
        render_all_registered_sliders();  // Render sliders
        render_all_registered_buttons();
		render_all_registered_texts();
        present_(&parent->base);
    }

    destroy_parent(parent);
}
