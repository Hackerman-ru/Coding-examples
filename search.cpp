#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>

const long double ERROR = 1e-9;

struct Comp {
    bool operator()(const std::string_view& lhs, const std::string_view& rhs) const {
        for (size_t i = 0; i < lhs.size() && i < rhs.size(); ++i) {
            if (std::tolower(lhs[i]) < std::tolower(rhs[i])) {
                return true;
            }

            if (std::tolower(lhs[i]) > std::tolower(rhs[i])) {
                return false;
            }
        }

        if (lhs.size() < rhs.size()) {
            return true;
        }

        return false;
    }
};

class SearchEngine {
public:
    struct RelevanceAndPos {
        double relevance = 0.0;
        size_t pos;
    };

    void BuildIndex(std::string_view text) {
        std::vector<std::vector<std::string_view>> words_in_lines;
        words_relevance_in_lines_.clear();
        ignore_list_.clear();
        lines_ = GetLinesFromText(text);
        text_words_ = GetUniqueWordsFromStringView(text);

        for (size_t i = 0; i < lines_.size(); ++i) {
            words_in_lines.emplace_back(GetWordsFromStringView(lines_[i]));

            if (words_in_lines[i].empty()) {
                ignore_list_.insert(i);
            }
        }

        std::unordered_map<std::string_view, std::vector<double>> word_tf_in_line;
        std::unordered_map<std::string_view, double> word_idf;

        for (size_t i = 0; i < words_in_lines.size(); ++i) {
            if (ignore_list_.find(i) != ignore_list_.end()) {
                continue;
            }

            auto temp = GetTFOfWordsInLine(words_in_lines[i], text_words_);

            for (const auto& word : text_words_) {
                word_tf_in_line[word].emplace_back(temp[word]);
                if (temp[word] != 0.0) {
                    ++word_idf[word];
                }
            }
        }

        for (const auto& word : text_words_) {
            if (word_idf[word] != 0.0) {
                word_idf[word] = std::log(static_cast<double>(lines_.size()) / word_idf[word]);
            }
        }

        for (size_t i = 0; i < lines_.size(); ++i) {
            if (ignore_list_.find(i) != ignore_list_.end()) {
                continue;
            }

            for (const auto& word : text_words_) {
                words_relevance_in_lines_[word].emplace_back(word_tf_in_line[word][i] * word_idf[word]);
            }
        }
    }

    std::vector<std::string_view> Search(std::string_view query, size_t results_count) const {
        std::vector<std::string_view> result;

        if (results_count == 0 || lines_.empty()) {
            return result;
        }

        std::set<std::string_view, Comp> query_words = GetUniqueWordsFromStringView(query);
        std::vector<RelevanceAndPos> lines_relevance;

        for (size_t i = 0; i < lines_.size(); ++i) {
            if (ignore_list_.find(i) != ignore_list_.end()) {
                continue;
            }

            RelevanceAndPos temp = {.relevance = 0., .pos = i};

            for (const auto& word : query_words) {
                if (auto it = words_relevance_in_lines_.find(word); it != words_relevance_in_lines_.end()) {
                    temp.relevance += it->second[i];
                }
            }

            lines_relevance.emplace_back(temp);
        }

        std::sort(lines_relevance.rbegin(), lines_relevance.rend(),
                  [](const RelevanceAndPos& lhs, const RelevanceAndPos& rhs) {
                      if (std::abs(lhs.relevance - rhs.relevance) < ERROR) {
                          return rhs.pos < lhs.pos;
                      }

                      return lhs.relevance < rhs.relevance;
                  });

        for (size_t i = 0; i < lines_relevance.size(); ++i) {
            if (i >= results_count || lines_relevance[i].relevance == 0.0) {
                break;
            }

            result.emplace_back(lines_[lines_relevance[i].pos]);
        }

        return result;
    }

private:
    std::set<std::string_view, Comp> text_words_;
    std::unordered_map<std::string_view, std::vector<double>> words_relevance_in_lines_;
    std::vector<std::string_view> lines_;
    std::set<size_t> ignore_list_;

    std::vector<std::string_view> GetWordsFromStringView(const std::string_view& str) const {
        size_t start_pos = 0;
        size_t finish_pos = start_pos;
        std::vector<std::string_view> result;

        while (start_pos < str.size()) {
            while (start_pos < str.size() && !std::isalpha(str[start_pos])) {
                ++start_pos;
            }

            if (start_pos == str.size()) {
                break;
            }

            finish_pos = start_pos + 1;

            while (finish_pos < str.size() && std::isalpha(str[finish_pos])) {
                ++finish_pos;
            }

            if (finish_pos == str.size()) {
                result.emplace_back(str.substr(start_pos));
                break;
            }

            result.emplace_back(str.substr(start_pos, finish_pos - start_pos));
            start_pos = finish_pos + 1;
        }

        return result;
    }

    std::set<std::string_view, Comp> GetUniqueWordsFromStringView(const std::string_view& str) const {
        size_t start_pos = 0;
        size_t finish_pos = start_pos;
        std::set<std::string_view, Comp> result;

        while (start_pos < str.size()) {
            while (start_pos < str.size() && !std::isalpha(str[start_pos])) {
                ++start_pos;
            }

            if (start_pos == str.size()) {
                break;
            }

            finish_pos = start_pos + 1;

            while (finish_pos < str.size() && std::isalpha(str[finish_pos])) {
                ++finish_pos;
            }

            if (finish_pos == str.size()) {
                result.insert(str.substr(start_pos));
                break;
            }

            result.insert(str.substr(start_pos, finish_pos - start_pos));
            start_pos = finish_pos + 1;
        }

        return result;
    }

    std::vector<std::string_view> GetLinesFromText(const std::string_view& text) const {
        size_t start_pos = 0;
        size_t finish_pos = 0;
        std::vector<std::string_view> result;

        do {
            finish_pos = text.find('\n', start_pos);

            if (finish_pos == start_pos) {
                ++start_pos;
                continue;
            }

            if (finish_pos == text.npos) {
                result.emplace_back(text.substr(start_pos));
                break;
            }

            result.emplace_back(text.substr(start_pos, finish_pos - start_pos));
            start_pos = finish_pos + 1;
        } while (start_pos < text.size());

        return result;
    }

    std::unordered_map<std::string_view, double> GetTFOfWordsInLine(
        const std::vector<std::string_view>& words_in_line, const std::set<std::string_view, Comp>& keywords) const {
        std::unordered_map<std::string_view, double> result;
        for (const auto& word : words_in_line) {
            if (const auto it = keywords.find(word); it != keywords.end()) {
                ++result[*it];
            }
        }

        for (const std::string_view& word : keywords) {
            result[word] /= static_cast<double>(words_in_line.size());
        }

        return result;
    }
};
