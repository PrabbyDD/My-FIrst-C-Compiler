#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./generation.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Incorrect usage. Correct Usage is..." << std::endl;
        std::cerr << "./hydro <input.hy>" << std::endl;
        return EXIT_FAILURE;
    }

    /*
     * Get File Contents before we start to tokenize the string
     * Make a string to hold our text eventually
     * Open a file with fstream, in input mode, the file is @ argv[1] (../test.hy for example)
     * Input is full if this is successful, then we read its buffer into the contents stream,
     * then we convert that content_stream into a string, and we have read our file into a string contents
     */
    std::string contents;
    // Do it in own scope so fstream closes when out of scope
    // Read in code as stream and convert to string we shall then convert to tokens
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    // Instead of move() we can just put the put contents in there, but I think this is better optimization wise
    // Convert string of code to tokens to then be parsed
    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();
    Parser parser(tokens);
    std::optional<NodeProg*> prog = parser.parse_prog();

    // Here if the parser returned no exit statement, then tree will be empty.
    if (!prog.has_value()) {
        std::cerr << "Invalid Program!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Similarly, value is a member of the optional class and returns the value which we use to fill a file with the correct assembly
    // tree.value() has the expression which has our exit code number
    Generator generator(prog.value());
    {
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog();
    }

    // C
    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
