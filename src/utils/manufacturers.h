// utils/manufacturers.h
#ifndef PINBALL_MANUFACTURERS_H
#define PINBALL_MANUFACTURERS_H

#include <vector>
#include <string>

namespace PinballManufacturers {

// Comprehensive manufacturer name list (all lowercase for matching).
// Longer or more specific names come first to improve detection accuracy.
// e.g. 'sonic pinball' before 'sonic'.
const std::vector<std::string> MANUFACTURERS_LOWERCASE = {
    // ------------------------------------------------------------------
    // Virtual Pinball / Digital Pinball Studios and Authors
    // ------------------------------------------------------------------
    "visual pinball",
    "future pinball",
    "zen studios",
    "zen pinball",
    "pinball fx",
    "pinball fx2",
    "pinball fx3",
    "pinball fx4",
    "pinball arcade",
    "the pinball arcade",
    "vpx original",
    "original",              // VPX original table (community)
    "recreational",          // often used by modders
    "mod",                   // MOD editions of tables
    "modded",
    "mod version",
    // "virtual pinball",
    // "vpx team",
    // "vpw",                   // Virtual Pin Workshop (VPW)
    // "vp universe",           // community site reference
    // "vpuniverse",
    // "pinup popper",          // launcher ecosystem
    // "pinup system",
    // "pinup player",
    // "b2s backglass",
    // "b2s team",
    // "slam tilt",             // notable Future Pinball creator
    // "ravarcade",             // Future Pinball engine author
    // "terryred",              // well-known FP/VP modder
    // "schreibi34",            // VPX modder
    // "balutito",              // VPX author
    // "bord",                  // VPX author
    // "hauntfreaks",           // VPX modder
    // "nailbuster",            // PinUP System creator
    // "loserman76",            // VPX table recreator (EM focus)
    // "jp salas",              // prolific VPX/Future Pinball author
    // "destruk",               // early VPX author
    // "bodydump",              // VPX modder
    // "cyberpez",              // VPX author
    // "stat",                  // VPW contributor
    // "schreibi",              // shorthand for schreibi34

    // ------------------------------------------------------------------
    // Major Historical Manufacturers (US)
    // ------------------------------------------------------------------
    "bally manufacturing",
    "bally midway",
    "williams electronics",
    "williams",
    "gottlieb",
    "premier technology",
    "data east",
    "stern pinball",
    "stern electronics",
    "stern",
    "midway manufacturing",
    "midway",
    "sega pinball",
    "segasa",                // Spanish variant (Segasa-Sonic)
    "sega",
    "atari",
    "capcom coin-op",
    "capcom",

    // ------------------------------------------------------------------
    // Other US Manufacturers
    // ------------------------------------------------------------------
    "chicago coin",
    "chicago gaming company",
    "chicago gaming",
    "game plan",
    "rock-ola",
    "rockola",
    "united manufacturing",
    "united",
    "exhibit supply",
    "keeney",
    "genco",
    "allied leisure",
    "alvin g and co",
    "alvin g",
    "grand products",
    "benchmark games",
    "american pinball",
    "jersey jack pinball",
    "jjp",
    "spooky pinball",
    "multimorphic",
    "haggis pinball",

    // ------------------------------------------------------------------
    // European Manufacturers
    // ------------------------------------------------------------------
    "zaccaria pinball",
    "zaccaria",
    "recel",
    "recreativos franco",
    "franco",
    "jeutel",
    "bell games",
    "inder",
    "nuova cei",
    "tecnoplay",
    "wico",
    "g&b",
    "mirco",
    "spm",
    "spinball",
    "taito do brasil",
    "taito",
    "playmatic",
    "mondial",
    "nsm",
    "replay",
    "sonic pinball",
    "sonic",
    "sankt georgen",
    "recel s.a.",
    "bingo gameroom",
    "euro pinball corp",

    // ------------------------------------------------------------------
    // Modern Indie / Homebrew / Virtual Projects
    // ------------------------------------------------------------------
    "silver castle pinball",
    "pedretti gaming",
    "retro pinball",
    "deep root pinball",
    "deeproot pinball",
    "homepin",
    "circus maximus",
    "p3",
    "multimorphic p3",
    "team pinball",
    "pinball brothers",
    "quetzal pinball",
    "valhalla pinball",
    "pinball adventures"
};

} // namespace PinballManufacturers

#endif // PINBALL_MANUFACTURERS_H
