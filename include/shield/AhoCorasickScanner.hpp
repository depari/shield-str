#pragma once
// shield/AhoCorasickScanner.hpp — O(N) multi-keyword scanner

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <queue>
#include <unordered_map>

namespace shield {

/**
 * @brief Aho-Corasick automaton for O(N) multi-keyword presence detection.
 *
 * Usage:
 *   AhoCorasickScanner scanner;
 *   scanner.build({"password", "token", "Bearer"});
 *   bool found = scanner.scan("Authorization: Bearer eyJ...");
 */
class AhoCorasickScanner {
public:
    AhoCorasickScanner() = default;

    /**
     * @brief Build the automaton from a list of trigger keywords.
     *        Must be called before scan().
     * @param keywords  Case-sensitive keyword list.
     */
    void build(const std::vector<std::string>& keywords);

    /**
     * @brief Scan text for any of the built keywords in O(N) time.
     * @return true if at least one keyword is present, false otherwise.
     */
    [[nodiscard]] bool scan(std::string_view text) const;

    /**
     * @return true if the automaton has been built with at least one keyword.
     */
    [[nodiscard]] bool empty() const noexcept { return num_states_ == 0; }

private:
    static constexpr int ALPHABET = 256;
    static constexpr int FAIL     = -1;

    struct State {
        std::array<int, ALPHABET> go;
        int                       fail   = 0;
        bool                      output = false;

        State() { go.fill(FAIL); }
    };

    std::vector<State> states_;
    int                num_states_ = 0;
};

} // namespace shield
