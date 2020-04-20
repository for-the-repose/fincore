#pragma once /*__ GPL 3.0, 2020 Alexander Soloviev (no.friday@yandex.ru) */

#include "file.h"
#include "tiny.h"
#include "ticks.h"
#include <random>
#include <vector>
#include <unistd.h>

class TMod_Write {

    struct TCfg {
        size_t Gran = 4096;
        uint64_t Delay = 0;     /* Delay between cunk writes, ms    */
        uint64_t Count = Max<uint64_t>();
        uint64_t Bytes = 0;
        uint64_t Sync = 0;      /* Zero disables data sync on write */
        bool Random = false;
        bool Direct = false;    /* Use direct IO                    */
        bool Evict = false;     /* Try to evict cache after sync    */
    };

public:
    int Handle(int argc, char *argv[])
    {
        extern char *optarg;

        std::string path;
        TCfg cfg{ };

        while (true) {
            static const char opts[] = "f:m:b:r:c:ds:u:e";

            const int opt = getopt(argc, argv, opts);

            if (opt < 0) break;

            if (opt == 'f') {
                path = optarg;
            } else if (opt == 'b') {
                cfg.Gran = std::stoull(optarg);
            } else if (opt == 's') {
                cfg.Bytes = std::stoull(optarg);
            } else if (opt == 'r') {
                cfg.Delay = std::stoull(optarg);
            } else if (opt == 'c') {
                cfg.Count = std::stoull(optarg);
            } else if (opt == 'd') {
                cfg.Direct = true;
            } else if (opt == 'e') {
                cfg.Evict = true;
            } else if (opt == 'u') {
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
        cfg.Bytes = NMisc::DivUp(cfg.Bytes, cfg.Gran) * cfg.Gran;

        if (cfg.Sync > 1024) {
            throw TError("For -e -u CYC have to be CYC <= 1024");
        }

        cfg.Evict = cfg.Evict && cfg.Sync > 0;

        return Run(path, cfg);
    }

    int Run(const std::string &path, const TCfg &cfg)
    {
        const uint64_t slots = NMisc::DivUp(cfg.Bytes, cfg.Gran);

        if (slots == 0 || cfg.Bytes == 0) {
            std::cerr << "Write file size=" << cfg.Bytes << " is invalid\n";

            return 3;
        }

        const size_t keys_num = 16;

        std::vector<uint64_t> offsets(cfg.Sync, Max<uint64_t>());
        std::vector<uint8_t*> key_store(keys_num, nullptr);

        for (size_t it = 0; it < keys_num; it++) {
            try {
                key_store[it] = (uint8_t*)NOs::MMap_Anon(cfg.Gran);

                NOs::TFile urand("/dev/urandom", false, true, false);

                if (NOs::Read(urand, key_store[it], cfg.Gran, 0, false) == 0) {
                    throw TError("Cannot fill sample buffer");
                }
            } catch (TError &err) {
                std::cerr
                    << "Cannot cook sample num=" << it << " " << cfg.Gran
                    << " bytes, errno=" << errno << ", " << err.what() << "\n";

                return 2;
            }
        }

        NOs::TFile  file;

        try {
            file = NOs::TFile(path, cfg.Direct, false, true);
        } catch (TError &error) {
            std::cerr << error.what() << std::endl;

            return 2;
        }

        if (auto error = ::posix_fallocate(file, 0, cfg.Bytes)) {
            std::cerr << "Cannot preallocate file, errno=" << error << "\n";

            return 2;
        }

        std::mt19937_64 entropy(7500);
        std::uniform_int_distribution<uint64_t> rnd(1, slots);
        std::uniform_int_distribution<size_t> key_sel(0, keys_num - 1);

        uint64_t unsynced_cycles = 0;
        auto pos = Max<uint64_t>(); /* current read position */

        for (NUtils::TTicks ti(cfg.Delay, cfg.Count); ti();) {
            pos = (pos + (cfg.Random ? rnd(entropy) : 1)) % slots;

            const auto *key = key_store[key_sel(entropy)];
            auto got = ::pwrite(file, key, cfg.Gran, pos * cfg.Gran);

            if (cfg.Evict) offsets[unsynced_cycles] = pos * cfg.Gran;

            if (got < 0 || size_t(got) != cfg.Gran) {
                std::cerr
                    << "Cannot write data, rv=" << got
                    << ", " << "errno=" << errno << "\n";

                return 2;
            } else if (cfg.Sync && ++unsynced_cycles >= cfg.Sync) {
                fdatasync(file); unsynced_cycles = 0;

                for (size_t num = 0; num < (cfg.Evict ? cfg.Sync : 0); num++) {
                    const auto mode = POSIX_FADV_DONTNEED;
                    posix_fadvise(file, offsets[num], cfg.Gran, mode);
                }
            }
        }

        return 0;
    }
};
