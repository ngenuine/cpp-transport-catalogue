#include "test_framework.h"
#include <stdexcept>

using namespace std::string_literals;
using namespace std::string_view_literals;

// Bus Ayn_Rand: Atlant raspravil plechi - Istochnik
// Bus Frank Herbert: Dune > Deti Dune > Messiya Dune > Bog Imperator Dune > Eritiki Dune > Capitul Dune
// Bus Den_Simmons: Giperion - Padenie Giperiona - Endimion - Voshod Endimiona
// Bus Ayzek Azimov: KonecVechnosti > Norbi > Ya robot > Akademiya > Akademiya i Imperiya > Vtoraya Akademiya > Akademiyz na krau gibeli > Akademiya i Zemlya

void AssertImpl(bool value, std::string_view expr_str, std::string_view file, std::string_view func, unsigned line,
                std::string_view hint) {
    if (!value) {
        std::cerr << file << "("sv << line << "): "sv << func << ": "sv;
        std::cerr << "ASSERT("sv << expr_str << ") failed."sv;
        if (!hint.empty()) {
            std::cerr << " Hint: "sv << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}