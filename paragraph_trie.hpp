#pragma once
#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Append-only word-prefix index. Every matching paragraph is returned once
// per insertion, in insertion order. ASCII case-insensitive; not Unicode-aware.
class ParagraphTrie {
    struct Node {
        std::map<unsigned char, std::unique_ptr<Node>> children;
        std::vector<std::size_t> paragraph_ids;
    };
    Node root_;
    std::vector<std::string> paragraphs_;

    // ASCII punctuation/whitespace separates words. Non-ASCII bytes are
    // preserved, so UTF-8 can be matched, but Unicode word boundaries are not.
    static bool word_byte(unsigned char c) noexcept {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c >= 128;
    }
    static unsigned char fold(unsigned char c) noexcept {
        return c >= 'A' && c <= 'Z'
            ? static_cast<unsigned char>(c + ('a' - 'A')) : c;
    }
    void index_word(std::string_view word, std::size_t id) {
        Node* node = &root_;
        for (unsigned char c : word) {
            const auto byte = fold(c);
            auto it = node->children.find(byte);
            if (it == node->children.end())
                it = node->children.emplace(byte, std::make_unique<Node>()).first;
            node = it->second.get();
            // IDs are added monotonically; repeated words/shared prefixes
            // within one paragraph must not duplicate that paragraph's ID.
            if (node->paragraph_ids.empty() || node->paragraph_ids.back() != id)
                node->paragraph_ids.push_back(id);
        }
    }

public:
    struct Match {
        std::size_t id;
        std::string paragraph; // Owned copy; safe after later insertions.
    };

    // Stores original capitalization and punctuation for display.
    std::size_t insert(std::string paragraph) {
        const auto id = paragraphs_.size();
        paragraphs_.push_back(std::move(paragraph));
        const std::string_view text = paragraphs_.back();
        std::size_t position = 0;
        while (position < text.size()) {
            while (position < text.size() &&
                   !word_byte(static_cast<unsigned char>(text[position])))
                ++position;
            const auto start = position;
            while (position < text.size() &&
                   word_byte(static_cast<unsigned char>(text[position])))
                ++position;
            if (position > start)
                index_word(text.substr(start, position - start), id);
        }
        return id;
    }

    // Query must be one nonempty word prefix; punctuation/spaces return no matches.
    [[nodiscard]] std::vector<Match> suggest(
        std::string_view prefix, std::size_t limit = 10) const {
        if (prefix.empty() || limit == 0) return {};
        const Node* node = &root_;
        for (unsigned char c : prefix) {
            if (!word_byte(c)) return {};
            const auto it = node->children.find(fold(c));
            if (it == node->children.end()) return {};
            node = it->second.get();
        }
        std::vector<Match> results;
        const auto count = std::min(limit, node->paragraph_ids.size());
        results.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto id = node->paragraph_ids[i];
            results.push_back({id, paragraphs_[id]});
        }
        return results;
    }

    [[nodiscard]] std::size_t size() const noexcept { return paragraphs_.size(); }
};
