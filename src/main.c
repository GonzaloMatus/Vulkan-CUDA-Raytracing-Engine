#include "app/application.h"

int main(void) {
    #ifndef NDEBUG
        printf("Starting application in DEBUG mode...\n");
    #endif
    Application app;
    app_init(&app);
    app_run(&app);
    app_shutdown(&app);
}
