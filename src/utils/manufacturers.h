// utils/manufacturers.h
#ifndef PINBALL_MANUFACTURERS_H
#define PINBALL_MANUFACTURERS_H

#include <vector>
#include <string>

namespace PinballManufacturers {

// A comprehensive list of pinball manufacturer names, all in lowercase for searching.
// This list is intended to be used for extracting manufacturer information from filenames.
// Repeated longer names come 1st (ex: 'taito do brasil' before 'taito')
const std::vector<std::string> MANUFACTURERS_LOWERCASE = {
    // Virtual Pinball Specific
    "original",

    // Major Historical Manufacturers (US)
    "bally",
    "williams",
    "gottlieb",
    "data east",
    "stern", // Covers both classic Stern Electronics and modern Stern Pinball
    "midway", // Acquired Bally/Williams
    "sega",   // Sega Pinball, later sold to Stern
    "atari",  // Atari Inc. (made some pinball machines)
    "capcom", // Capcom Coin-Op (made some pinball machines)

    // Other US Manufacturers
    "chicago coin", // Early prominent manufacturer
    "game plan",
    "rock-ola",
    "united",
    "exhibit supply",
    "keeney",
    "genco",
    "allied leisure",
    "alvin g. and co.",
    "grand products",
    "benchmark games", // Modern, often redemption but some pinball history
    "chicago gaming company", // CGC - modern remakes
    "american pinball",       // AP - modern
    "jersey jack pinball",    // JJP - modern
    "spooky pinball",         // Modern
    "multimorphic",           // P3 system - modern
    "haggis pinball",         // Modern (Australian, but widely known)

    // European Manufacturers
    "zaccaria pinball"
    "zaccaria",         // Italian
    "recreativos franco", // Spanish (often just "Franco")
    "franco",
    "jeutel",           // French
    "bell games",       // UK
    "inder",            // Spanish
    "nuova cei",        // Italian
    "tecnoplay",        // Italian
    "wico",             // German
    "g & b",            // German (also "g&b")
    "mirco",            // Mirco Games, German
    "s.p.m.",           // Italian

    // Other / Niche / Modern Virtual
    "premier", // Premier Technology (Gottlieb successor)
    "spinball",
    "culik pinball",
    "taito do brasil",
    "taito",
    "playmatic"
};

} // namespace PinballManufacturers

#endif //PINBALL_MANUFACTURERS_H