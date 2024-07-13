#include <cstdlib> 
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <memory>

using namespace std;

// generador de valores flotantes aleatorios en un arreglo donde cada elemento toma 3 espacios: ej, el elemento i tiene componentes en arr[i], arr[i+1], arr[i+2]
void init_values(int x_limit, int y_limit, int z_limit, float *arr, int arr_size) {
  // srand(time(0));

  for (int i = 0; i < arr_size*6; i+=6) {
    arr[i] = (float)((rand() % (2*x_limit)) - x_limit) + (float) rand()/RAND_MAX;
    arr[i+1] = (float)((rand() % (2*y_limit)) - y_limit) + (float) rand()/RAND_MAX;
    arr[i+2] = (float)((rand() % (2*z_limit)) - z_limit) + (float) rand()/RAND_MAX;
    arr[i+3] = (float) rand()/RAND_MAX;
    arr[i+4] = (float) rand()/RAND_MAX;
    arr[i+5] = (float) rand()/RAND_MAX;
  }

}

std::string load_from_file(const std::string &path) {

    auto close_file = [](FILE *f) { fclose(f); };

    auto holder = std::unique_ptr<FILE, decltype(close_file)>(fopen(path.c_str(), "rb"), close_file);
    if (!holder)
        return "";

    FILE *f = holder.get();

    // in C++17 following lines can be folded into std::filesystem::file_size invocation
    if (fseek(f, 0, SEEK_END) < 0)
        return "";

    const long size = ftell(f);
    if (size < 0)
        return "";

    if (fseek(f, 0, SEEK_SET) < 0)
        return "";

    std::string res;
    res.resize(size);

    // C++17 defines .data() which returns a non-const pointer
    fread(const_cast<char *>(res.data()), 1, size, f);

    return res;
}