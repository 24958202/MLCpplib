#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

// ── Function template definition ─────────────────────────
template<typename Callback>
bool get_word_type(
    const std::string& word,
    const std::unordered_map<std::string, std::string>& types,
    Callback&& callback)          // Accepts any callable object
{
    auto it = types.find(word);
    if (it == types.end()) return false;

    // Build result vector and invoke the callback
    std::vector<std::string> result = { word, it->second, "example_en", "example_zh" };
    callback(result, types);      // Direct call — zero vtable overhead
    return true;
}

// ── Callback function ─────────────────────────────────────
void my_callback(
    const std::vector<std::string>& result,
    const std::unordered_map<std::string, std::string>& word_types)
{
    std::cout << "[A] word      : " << result[0] << "\n"
              << "[A] word_type : " << result[1] << "\n"
              << "[A] English   : " << result[2] << "\n"
              << "[A] Chinese   : " << result[3] << "\n";
}

int main()
{
    std::unordered_map<std::string, std::string> types = {
        {"run",  "verb"},
        {"fast", "adjective"}
    };

    // Pass a plain function pointer
    get_word_type("run", types, my_callback);

    // A lambda can also be passed directly (Method A supports this too)
    get_word_type("fast", types,
        [](const std::vector<std::string>& r,
           const std::unordered_map<std::string,std::string>&) {
            std::cout << "[A-lambda] " << r[0] << " is " << r[1] << "\n";
        });
}


--------------------------------------------------------------------------------------


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <concepts>          // ← C++20 required

// ── Type aliases (optional — keeps the concept constraint concise) ──
using ResultVec  = std::vector<std::string>;
using TypeMap    = std::unordered_map<std::string, std::string>;

// ── Function template with concept constraint ─────────────
template<std::invocable<const ResultVec&, const TypeMap&> Callback>
bool get_word_type(
    const std::string& word,
    const TypeMap& types,
    Callback&& callback)
{
    auto it = types.find(word);
    if (it == types.end()) return false;

    ResultVec result = { word, it->second, "example_en", "example_zh" };
    std::invoke(std::forward<Callback>(callback), result, types); // C++20 best practice
    return true;
}

// ── Callback function
// (signature must satisfy the concept, otherwise a compile-time error is raised) ──
void my_callback(const ResultVec& result, const TypeMap& word_types)
{
    std::cout << "[B] word      : " << result[0] << "\n"
              << "[B] word_type : " << result[1] << "\n"
              << "[B] English   : " << result[2] << "\n"
              << "[B] Chinese   : " << result[3] << "\n";
}

// ── Demo: a mismatched signature is rejected at compile time ──
// void bad_callback(int x) {}   // ← passing this would produce a clear concept error

int main()
{
    TypeMap types = {
        {"run",  "verb"},
        {"fast", "adjective"}
    };

    // Pass a plain function
    get_word_type("run", types, my_callback);

    // Pass a lambda (the concept validates its signature too)
    get_word_type("fast", types,
        [](const ResultVec& r, const TypeMap&) {
            std::cout << "[B-lambda] " << r[0] << " is " << r[1] << "\n";
        });

    // Pass a functor (function object)
    struct MyFunctor {
        void operator()(const ResultVec& r, const TypeMap&) const {
            std::cout << "[B-functor] " << r[0] << " -> " << r[1] << "\n";
        }
    };
    get_word_type("run", types, MyFunctor{});
}
