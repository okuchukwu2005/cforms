// main.c
#include <stdio.h>
#include <stdbool.h>
#include "forms/form.h"
int main(void) {
    App app = init();
    app.window = new_window_("My Window", 1000, 700);
    
    Parent* container = new_container_(app.window, 10, 10, 360, 500);

    // Enable moving, title bar, close button,
    set_container_properties_(container, true, "My Container", true, true);

    Parent* container2 = new_container_(app.window, 400, 10, 400, 650);

    // Enable moving, title bar, close button,
    set_container_properties_(container2, true, "My Second Container", true, true);

    	const char* content = "This is a Demo of c forms";
    Text* text = new_text_(container, 0,0, content, 16, ALIGN_LEFT);
        // ---------- RADIO BUTTON DEMO ----------
    // Group 1 (only one of these three can be selected at a time)
    new_radio_button_(container, 15, 100, 20, 20, "Option 1", 1);
    new_radio_button_(container, 15, 130, 20, 20, "Option 2", 1);
    new_radio_button_(container, 15, 160, 20, 20, "Option 3", 1);

    // Group 2 (independent from group 1)
    new_radio_button_(container, 200, 100, 20, 20, "Choice A", 2);
    new_radio_button_(container, 200, 130, 20, 20, "Choice B", 2);

       // Create dropdown options
         char* options[] = {"Option 1", "Option 2", "Option 3", "Option 4"};
         int option_count = 4;
         Drop* dropdown = new_drop_down_(container, 15, 200, 170, 30, options, option_count);
         
// ---------- SLIDER DEMO ----------
    // Create a slider in container (horizontal, 100px wide, 20px high, range 0-100, starting at 50)
    Slider* slider1 = new_slider(container, 10, 370, 250, 15, 0, 100, 50, "Volume");

 	 Entry* entry =  new_entry_(container2, 20, 70, 300, 2048);
 	 entry->place_holder = "enter text";

// 
     TextBox* texty = new_textbox_(container2, 20, 150, 300, 1024);
// 
   	Button* button = new_button_(container2, 100, 400, 120, 40, "Click Me", OVERRIDE);

	ProgressBar* prox = new_progress_bar_(container2, 20, 500, 250, 15, 0, 100, 50, true);

    

    app_run_(app.window);

    // Free resources (add sliders if you implement a free_slider function)
    // free_slider(slider1);
    // free_slider(slider2);
    // free_drop_(dropdown);
    // free_con_(container);
    // free_con_(container2);

    return 0;
}

// Modified app_run_ function in form.h or wherever it's defined
