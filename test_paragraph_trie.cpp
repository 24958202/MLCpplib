#include "paragraph_trie.hpp"
#include <iostream>

int main() {
    ParagraphTrie notes;

    notes.insert("Tomorrow we have a meeting with the team.");
    notes.insert("Please prepare the meeting notes.");
    notes.insert("Shopping: milk, bread, and eggs.");

    for (const auto& match : notes.suggest("me", 5)) {
        std::cout << match.paragraph << '\n';

        // match.id identifies the original paragraph.
    }
}
