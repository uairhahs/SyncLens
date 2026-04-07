#include "app/Application.h"

#include <cstdlib>

int main(int argc, char *argv[])
{
    // Qt WebEngine (Chromium) crashes on systems where GBM/Vulkan are
    // unavailable (common on X11). Disable GPU; software rasterizer takes over.
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu");

    Application app(argc, argv);
    return app.run();
}
