#include "app/application.h"
#include <iostream>
#include <exception>

int main()
{
    try
    {
        const int WINDOW_WIDTH  = 1600;
        const int WINDOW_HEIGHT = 900;

        App::Application app(WINDOW_WIDTH, WINDOW_HEIGHT);
        app.run();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
