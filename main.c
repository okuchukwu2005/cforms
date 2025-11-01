// main.c
#include <stdio.h>
#include <stdbool.h>
#include "forms/form.h"
int main(void) {
    App app = init_app();
    app.window = new_window("My Window", 1000, 700);
    Parent container = new_container(&app.window, 10, 10, 360, 500);

    // Enable moving, title bar, close button,
    set_container_properties(&container, true, "My Container", true, true);
    register_container(&container);

    Parent container2 = new_container(&app.window, 400, 10, 400, 650);

    // Enable moving, title bar, close button,
    set_container_properties(&container2, true, "My Second Container", true, true);
    register_container(&container2);

    	const char* content = "This is a Demo of c forms";
    Text text = new_text(&container, 0,0, content, 16, ALIGN_LEFT);
    // Register widget
    register_text(&text);

        // ---------- RADIO BUTTON DEMO ----------
    // Group 1 (only one of these three can be selected at a time)
    new_radio_button_(&container, 15, 100, 20, 20, "Option 1", 1);
    new_radio_button_(&container, 15, 130, 20, 20, "Option 2", 1);
    new_radio_button_(&container, 15, 160, 20, 20, "Option 3", 1);

    // Group 2 (independent from group 1)
    new_radio_button_(&container, 200, 100, 20, 20, "Choice A", 2);
    new_radio_button_(&container, 200, 130, 20, 20, "Choice B", 2);

       // Create dropdown options
         char* options[] = {"Option 1", "Option 2", "Option 3", "Option 4"};
         int option_count = 4;
         Drop drop_down = new_drop_down(&container, 15, 200, 170, 30, options, option_count);
         register_drop(&drop_down);
// ---------- SLIDER DEMO ----------
    // Create a slider in container (horizontal, 100px wide, 20px high, range 0-100, starting at 50)
    Slider* slider1 = new_slider(&container, 10, 370, 250, 15, 0, 100, 50, "Volume");

 	 Entry* entry =  new_entry_(&container2, 20, 70, 300, 2048);
 	 set_entry_placeholder(entry, "enter text");

// 
     TextBox* texty = new_textbox_(&container2, 20, 150, 300, 1024);
// 
   	Button* button = new_button_(&container2, 100, 400, 120, 40, "Click Me", OVERRIDE);

	ProgressBar* prox = new_progress_bar_(&container2, 20, 500, 250, 15, 0, 100, 50, true);

    

    app_run_(&app.window);


    return 0;
}
