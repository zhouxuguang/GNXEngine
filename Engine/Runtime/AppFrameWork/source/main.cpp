#include "MTLGLFWFramework.h"

int main(void)
{
    MTLGLFWFramework app(640, 480, "GLFW Metal");
    app.exec();
    return 0;
}
