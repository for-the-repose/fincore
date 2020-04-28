#pragma once /*__ GPL 3.0, 2020 Alexander Soloviev (no.friday@yandex.ru) */

#include "file.h"
#include "tiny.h"
#include "ticks.h"
#include <random>
#include <unistd.h>

class TMod_Read {

    struct TCfg {
        size_t Gran = 4096;
        uint64_t Delay = 0;       /* milliseconds */
        uint64_t Count = Max<uint64_t>();
        uint64_t Sync = 0;      /* Zero disables data sync on write */
        bool Random = false;
        bool Direct = false;    /* Use direct IO */
    };

public:
    int Handle(int argc, char *argv[])
    {
        extern char *optarg;

        std::string path;
        TCfg cfg{ };

        while (true) {
            static const char opts[] = "f:m:b:r:c:de:";

            const int opt = getopt(argc, argv, opts);

            if (opt < 0) break;

            if (opt == 'f') {
                path = optarg;
            } else if (opt == 'b') {
                cfg.Gran = std::stoull(optarg);
            } else if (opt == 'r') {
                cfg.Delay = std::stoull(optarg);
            } else if (opt == 'c') {
                cfg.Count = std::stoull(optarg);
            } else if (opt == 'd') {
                cfg.Direct = true;
            } else if (opt == 'e') {
                cfg.Sync = std::stoull(optarg);
            } else if (opt == 'm') {
                const std::string rname(optarg);

                if (rname == "seq") {
                    cfg.Random = false;
                } else if (rname == "rnd") {
                    cfg.Random = true;
                } else {
                    std::cerr << "unknown read mode " << rname << std::endl;

                    return 1;
                }
            }
        }

        cfg.Gran = NMisc::DivUp(cfg.Gran, 4096) * 4096;

        return Run(path, cfg);
    }

    int Run(const std::string &path, const TCfg &cfg)
    {
        NOs::TFile  file;

        try {
            file = NOs::TFile(path, cfg.Direct);
        } catch (TError &error) {
            std::cerr << error.what() << std::endl;

            return 2;
        }

        std::vector<uint64_t> offsets(cfg.Sync, Max<uint64_t>());

        const uint64_t bytes = NOs::TStat(file).Bytes;
        const uint64_t slots = NMisc::DivUp(bytes, cfg.Gran);

        if (slots == 0) return 3;

        std::mt19937_64 entropy(7500);
        std::uniform_int_distribution<uint64_t> rnd(1, slots);

        auto *buf = (uint8_t*)NOs::MMap_Anon(cfg.Gran);

        uint64_t unsynced_cycles = 0;
        auto pos = Max<uint64_t>(); /* current read position */

        for (NUtils::TTicks ti(cfg.Delay, cfg.Count); ti();) {
            pos = (pos + (cfg.Random ? rnd(entropy) : 1)) % slots;

            if (cfg.Sync) offsets[unsynced_cycles++] = pos * cfg.Gran;

            if (!NOs::Read(file, buf, cfg.Gran, pos * cfg.Gran, cfg.Direct)) {
                return 2;
            } else if (cfg.Sync && unsynced_cycles >= cfg.Sync) {
                const auto slots = std::exchange(unsynced_cycles, 0);

                for (size_t num = 0; num < slots; num++) {
                    const auto mode = POSIX_FADV_DONTNEED;
                    posix_fadvise(file, offsets[num], cfg.Gran, mode);
                }
            }
        }

        return 0;
    }

};
