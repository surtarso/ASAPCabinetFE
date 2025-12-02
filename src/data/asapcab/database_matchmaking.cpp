#include "database_matchmaking.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace data::asapcabdb {

int levenshtein(const std::string& s1, const std::string& s2) {
    const int m = static_cast<int>(s1.size());
    const int n = static_cast<int>(s2.size());
    std::vector<std::vector<int>> dp(static_cast<size_t>(m) + 1,
                                     std::vector<int>(static_cast<size_t>(n) + 1, 0));
    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[m][n];
}

double calculateSimilarity(const std::string& s1, const std::string& s2) {
    if (s1.empty() && s2.empty()) return 1.0;
    if (s1.empty() || s2.empty()) return 0.0;
    int dist = levenshtein(s1, s2);
    int maxLen = static_cast<int>(std::max(s1.length(), s2.length()));
    return 1.0 - (static_cast<double>(dist) / maxLen);
}

std::string normalize(const std::string& s) {
    std::string result;
    std::transform(s.begin(), s.end(), std::back_inserter(result), ::tolower);
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](char c) { return !isalnum(static_cast<unsigned char>(c)); }),
                 result.end());
    return result;
}

std::string normalizeForMatching(const std::string& s) {
    std::string result = normalize(s);
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](char c) { return isspace(static_cast<unsigned char>(c)); }),
                 result.end());
    return result;
}

int extractYear(const std::string& dateStr) {
    if (dateStr.length() >= 4) {
        try {
            int year = std::stoi(dateStr.substr(0, 4));
            if (year >= 1970 && year <= 2100) return year;
        } catch (...) {}
    }
    return 0;
}

std::optional<std::string> safeGetString(const json& obj, const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        if (obj.contains(key)) {
            try {
                if (obj[key].is_string()) {
                    std::string val = obj[key].get<std::string>();
                    if (!val.empty()) return val;
                }
            } catch (...) {
                continue;
            }
        }
    }
    return std::nullopt;
}

std::optional<int> safeGetInt(const json& obj, const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        if (obj.contains(key)) {
            try {
                if (obj[key].is_number()) {
                    return obj[key].get<int>();
                } else if (obj[key].is_string()) {
                    std::string str = obj[key].get<std::string>();
                    if (!str.empty()) {
                        return std::stoi(str);
                    }
                }
            } catch (...) {
                continue;
            }
        }
    }
    return std::nullopt;
}

void MatchScore::calculate() {
    totalScore = (nameScore * 0.40) + (yearScore * 0.20) + (manufacturerScore * 0.20) +
                 (playerCountScore * 0.10) + (authorScore * 0.10);
}

void CandidateNames::add(const std::string& s) {
    if (!s.empty() && std::find(names.begin(), names.end(), s) == names.end()) {
        names.push_back(s);
    }
}

// Field mapping definitions
std::vector<std::string> TableMatcher::getNameFields(DatabaseSource db) {
    switch (db) {
        case DatabaseSource::VPSDB:
            return {"name", "title"};
        case DatabaseSource::IPDB:
            return {"Title", "title"};
        case DatabaseSource::LBDB:
            return {"Name", "name"};
        // case DatabaseSource::VPINMDB:
        //     return {"gameName", "name"};
        default:
            return {"name"};
    }
}

std::vector<std::string> TableMatcher::getManufacturerFields(DatabaseSource db) {
    switch (db) {
        case DatabaseSource::VPSDB:
            return {"manufacturer", "company"};
        case DatabaseSource::IPDB:
            return {"ManufacturerShortName", "Manufacturer", "manufacturer"};
        case DatabaseSource::LBDB:
            return {"Manufacturer", "manufacturer", "Publisher"};
        // case DatabaseSource::VPINMDB:
        //     return {"manufacturer", "company"};
        default:
            return {"manufacturer"};
    }
}

std::vector<std::string> TableMatcher::getYearFields(DatabaseSource db) {
    switch (db) {
        case DatabaseSource::VPSDB:
            return {"year", "releaseYear"};
        case DatabaseSource::IPDB:
            return {"DateOfManufacture", "Year", "year"};
        case DatabaseSource::LBDB:
            return {"Year", "year"};
        // case DatabaseSource::VPINMDB:
        //     return {"year", "releaseYear"};
        default:
            return {"year"};
    }
}

std::vector<std::string> TableMatcher::getPlayerCountFields(DatabaseSource db) {
    switch (db) {
        case DatabaseSource::VPSDB:
            return {"playerCount", "players"};
        case DatabaseSource::IPDB:
            return {"MaxPlayersAllowed", "playerCount", "Players"};
        // case DatabaseSource::LBDB:
        //     return {"PlayerCount", "playerCount"};
        // case DatabaseSource::VPINMDB:
        //     return {"playerCount", "players"};
        default:
            return {"playerCount"};
    }
}

std::vector<std::string> TableMatcher::getAuthorFields(DatabaseSource db) {
    switch (db) {
        case DatabaseSource::VPSDB:
            return {"author", "designer", "authors"};
        case DatabaseSource::IPDB:
            return {"Designer", "author"};
        // case DatabaseSource::LBDB:
        //     return {"Author", "author", "designer"};
        // case DatabaseSource::VPINMDB:
        //     return {"author", "designer"};
        default:
            return {"author"};
    }
}

MatchScore TableMatcher::scoreMatch(const json& source,
                                    const json& target,
                                    const CandidateNames& sourceNames,
                                    DatabaseSource sourceDb,
                                    DatabaseSource targetDb) {
    MatchScore score;

    // Name matching: try all source candidate names (including provided candidates),
    // normalize both sides before similarity to reduce false negatives.
    auto targetNameOpt = safeGetString(target, getNameFields(targetDb));
    if (targetNameOpt) {
        std::string targetNameNorm = normalizeForMatching(targetNameOpt.value());
        double bestNameScore = 0.0;

        // Try explicit source fields first
        auto sourceNameOpt = safeGetString(source, getNameFields(sourceDb));
        if (sourceNameOpt) {
            bestNameScore = std::max(bestNameScore, calculateSimilarity(normalizeForMatching(sourceNameOpt.value()), targetNameNorm));
        }

        // Try candidate names (aliases/titles) if provided
        for (const auto& cand : sourceNames.names) {
            if (cand.empty()) continue;
            bestNameScore = std::max(bestNameScore, calculateSimilarity(normalizeForMatching(cand), targetNameNorm));
        }

        score.nameScore = bestNameScore;
    }

    // Year matching (handle date strings with extractYear)
    auto sourceYearField = safeGetString(source, getYearFields(sourceDb));
    auto targetYearField = safeGetString(target, getYearFields(targetDb));
    int sourceYear = 0;
    int targetYear = 0;

    if (sourceYearField) {
        auto yearInt = safeGetInt(source, getYearFields(sourceDb));
        sourceYear = yearInt.value_or(extractYear(sourceYearField.value()));
    }
    if (targetYearField) {
        auto yearInt = safeGetInt(target, getYearFields(targetDb));
        targetYear = yearInt.value_or(extractYear(targetYearField.value()));
    }

    if (sourceYear != 0 && targetYear != 0) {
        score.yearScore = (sourceYear == targetYear) ? 1.0 : 0.0;
    }

    // Manufacturer matching - normalize strings before comparing
    auto sourceManuf = safeGetString(source, getManufacturerFields(sourceDb));
    auto targetManuf = safeGetString(target, getManufacturerFields(targetDb));
    if (sourceManuf && targetManuf) {
        score.manufacturerScore = calculateSimilarity(normalizeForMatching(sourceManuf.value()), normalizeForMatching(targetManuf.value()));
    }

    // Player count matching
    auto sourcePC = safeGetInt(source, getPlayerCountFields(sourceDb));
    auto targetPC = safeGetInt(target, getPlayerCountFields(targetDb));
    if (sourcePC && targetPC) {
        score.playerCountScore = (sourcePC.value() == targetPC.value()) ? 1.0 : 0.0;
    }

    // Author matching - normalize before comparing
    auto sourceAuthor = safeGetString(source, getAuthorFields(sourceDb));
    auto targetAuthor = safeGetString(target, getAuthorFields(targetDb));
    if (sourceAuthor && targetAuthor) {
        score.authorScore = calculateSimilarity(normalizeForMatching(sourceAuthor.value()), normalizeForMatching(targetAuthor.value()));
    }

    score.calculate();
    return score;
}

} // namespace data::asapcabdb
