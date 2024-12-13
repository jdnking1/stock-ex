#include "main.hpp"
#include "matching_engine.hpp"

std::vector<std::string> run(std::vector<std::string> const& input) {
    auto engine = engine::Engine();
    engine.run(input);
    return engine.generateOutput();
}