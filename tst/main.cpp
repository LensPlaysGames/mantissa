#include <iostream>
#include <filesystem>

/// ./testrunner --test <filepath>
int main(int argc, char **argv) {
    assert(argc == 3, "There must be two arguments: `--test` followed by a filepath");
    if (strcmp(argv[1], "--test") != 0) {
        std::cout << "ERROR: First argument must be `--test`\n";
        return -1;
    }
    std::filesystem::path testpath{argv[2]};
}
