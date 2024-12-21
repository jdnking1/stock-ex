#include <iostream>
#include <uv.h>

int main() {
    uv_loop_t* loop = uv_default_loop();

    std::cout << "Libuv version: " << uv_version_string() << std::endl;

    return uv_run(loop, UV_RUN_DEFAULT);
}