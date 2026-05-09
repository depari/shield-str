// shield/AhoCorasickScanner.cpp

#include "shield/AhoCorasickScanner.hpp"
#include <queue>

namespace shield {

void AhoCorasickScanner::build(const std::vector<std::string>& keywords) {
    if (keywords.empty()) {
        num_states_ = 0;
        states_.clear();
        return;
    }

    // Root state = index 0
    states_.clear();
    states_.emplace_back();  // state 0 = root
    num_states_ = 1;

    // --- Build goto function (trie) ---
    for (const auto& keyword : keywords) {
        if (keyword.empty()) continue;
        int cur = 0;
        for (unsigned char c : keyword) {
            if (states_[cur].go[c] == FAIL) {
                states_[cur].go[c] = num_states_++;
                states_.emplace_back();
            }
            cur = states_[cur].go[c];
        }
        states_[cur].output = true;
    }

    // --- Build failure function via BFS ---
    std::queue<int> q;

    // Depth-1 states: fail → root(0)
    for (int c = 0; c < ALPHABET; ++c) {
        int s = states_[0].go[c];
        if (s == FAIL) {
            states_[0].go[c] = 0;  // loop back to root
        } else {
            states_[s].fail = 0;
            q.push(s);
        }
    }

    while (!q.empty()) {
        int r = q.front(); q.pop();
        // Propagate output flag along failure links
        if (states_[states_[r].fail].output) {
            states_[r].output = true;
        }
        for (int c = 0; c < ALPHABET; ++c) {
            int s = states_[r].go[c];
            if (s == FAIL) {
                // Follow failure link
                states_[r].go[c] = states_[states_[r].fail].go[c];
            } else {
                states_[s].fail = states_[states_[r].fail].go[c];
                q.push(s);
            }
        }
    }
}

bool AhoCorasickScanner::scan(std::string_view text) const {
    if (num_states_ == 0) return false;

    int cur = 0;
    for (unsigned char c : text) {
        cur = states_[cur].go[c];
        if (states_[cur].output) return true;
    }
    return false;
}

} // namespace shield
