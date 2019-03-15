/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <unistd.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>

#include "probe.h"
#include "diff.h"
#include "print.h"
#include "ticks.h"
#include "parts.h"

class TMonit {
public:
    using TTicks = NUtils::TTicks<>;
    using TSampled = NStats::TParted<NParts::Tailed>;

    class TCfg {
    public:
        unsigned    delay   = 0;
        size_t      count   = 1;
        float       thresh  = 0.1;
        unsigned    bands   = 48;
        unsigned    subs    = 8192;
    };

    TMonit(const TCfg &cfg_) : cfg(cfg_) { }

    void Do(std::string &path)
    {
        TProbe probe;
        NParts::TScale scale(cfg.subs);

        TSampled::Ref  was;

        for (TTicks ti(cfg.delay * 1000, cfg.count); ti();) {
            NOs::TFile  file;

            try {
                file = NOs::TFile(path);
            } catch (TError &error) {
                std::cerr << error.what() << std::endl;

                break;
            }

            auto map = file.mmap();

            const size_t bytes = ((const NOs::TMemRg&)map).paged();

            TSampled::Ref now(new TSampled(bytes, scale(bytes)));

            probe(map, [&](NUtils::TSpan &span) { (*now)(span); });

            if (!was || NStats::TDiff()(*was, *now) > cfg.thresh) {
                was.reset(now.release());

                std::cout
                    << Stamp()
                    << " "
                    << NStats::TPrint(*was, cfg.bands)
                    << std::endl;
            }
        }
    }

    std::string Stamp() const noexcept
    {
        using namespace std;

        time_t now = time(nullptr);
        tm parts;

        memset(&parts, 0, sizeof(parts));

        localtime_r(&now, &parts);
        
        char line[64];

        strftime(line, sizeof(line), "%m-%d %H:%M:%S", &parts);

        return line;
    }

protected:
    const TCfg      &cfg;
};

