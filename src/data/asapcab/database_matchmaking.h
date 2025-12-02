#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>

using json = nlohmann::json;

namespace data::asapcabdb {

// Calculate Levenshtein distance
int levenshtein(const std::string& s1, const std::string& s2);

// Calculate normalized string similarity (0.0 to 1.0)
double calculateSimilarity(const std::string& s1, const std::string& s2);

// Normalize string to lowercase alphanumeric
std::string normalize(const std::string& s);

// Normalize and compress whitespace for matching
std::string normalizeForMatching(const std::string& s);

// Extract year from date string (first 4 chars)
int extractYear(const std::string& dateStr);

// Safe JSON value extraction with fallback keys
std::optional<std::string> safeGetString(const json& obj, const std::vector<std::string>& keys);
std::optional<int> safeGetInt(const json& obj, const std::vector<std::string>& keys);

// Scoring structure for match results
struct MatchScore {
    double nameScore = 0.0;
    double yearScore = 0.0;
    double manufacturerScore = 0.0;
    double playerCountScore = 0.0;
    double authorScore = 0.0;
    double totalScore = 0.0;

    void calculate();
};

// Candidate names for multi-factor matching
struct CandidateNames {
    std::vector<std::string> names;
    void add(const std::string& s);
};

// Main matcher class with field mapping support
class TableMatcher {
public:
    // expose the top-level CandidateNames as a nested type for compatibility
    using CandidateNamesAlias = ::data::asapcabdb::CandidateNames;
    // add the specific alias name used by callers
    using CandidateNames = CandidateNamesAlias;

    // Database source enum for field mapping
    enum class DatabaseSource {
        VPSDB,
        IPDB,
        LBDB,
        VPINMDB
    };

    static MatchScore scoreMatch(const json& source,
                                 const json& target,
                                 const CandidateNames& sourceNames,
                                 DatabaseSource sourceDb = DatabaseSource::VPSDB,
                                 DatabaseSource targetDb = DatabaseSource::VPSDB);

private:
    // Field name mappings per database
    static std::vector<std::string> getNameFields(DatabaseSource db);
    static std::vector<std::string> getManufacturerFields(DatabaseSource db);
    static std::vector<std::string> getYearFields(DatabaseSource db);
    static std::vector<std::string> getPlayerCountFields(DatabaseSource db);
    static std::vector<std::string> getAuthorFields(DatabaseSource db);
};

} // namespace data::asapcabdb
