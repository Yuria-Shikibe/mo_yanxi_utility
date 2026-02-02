#include <cstdio>

import std;
import mo_yanxi.static_string;

using namespace mo_yanxi;

int main() {
    constexpr static static_string<8> test{"off"};
    constexpr auto idx1 = std::bit_cast<std::uint64_t>(test.get_data());
    auto idx2 = std::bit_cast<std::uint64_t>(test.get_data());
    auto c = idx1 == idx2;
    return 0;
}