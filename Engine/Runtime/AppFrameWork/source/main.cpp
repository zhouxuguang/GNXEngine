#include "AppFrameWork.h"

int main(void)
{
    AppFrameWork *app = AppFrameWork::Create(640, 480, "GLFW");
    app->exec();
    return 0;
}
