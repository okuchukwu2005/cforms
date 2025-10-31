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

       // Create dropdown options
         char* options[] = {"Option 1", "Option 2", "Option 3", "Option 4"};
         int option_count = 4;
         Drop* dropdown = new_drop_down_(container, 300, 200, 170, 30, options, option_count);
         Drop* dropdown1 = new_drop_down_(app.window, 400, 200, 170, 30, options, option_count);
         Drop* dropdown2 = new_drop_down_(container, 40, 200, 170, 30, options, option_count);
         
// ---------- SLIDER DEMO ----------
 
    

    app_run_(app.window);


    return 0;
}
