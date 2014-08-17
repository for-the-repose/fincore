/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_MONIT
#define H_FINCORE_MONIT

#include <unistd.h>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>

#include "probe.h"
#include "stats.h"

class Monit {
public:
    class Cfg {
    public:
        unsigned    delay   = 0;
        size_t      count   = 1;
        float       thresh  = 0.1;
    };

    Monit(const Cfg &cfg_) : cfg(cfg_) { }

    void Do(std::string &path)
    {
        Probe probe;

        const std::chrono::seconds duration(cfg.delay);

        Stats::Bands::Ref  was;

        for(unsigned count = cfg.count; count > 0; count--) {
            OS::File  file;

            try {
                file = OS::File(path);

            } catch (Error &error) {
                std::cerr << error.what() << std::endl;

                break;
            }

            auto map = file.mmap();

            Stats::Bands::Ref now(new Stats::Bands(map, 48));

            const OS::MemRg mem = map;

            probe(mem, [&](Utils::Span &span) { (*now)(span); });

            if (!was.get() || was->diff(*now, cfg.thresh)) {
                putStamp();

                std::cerr << " ";

                was.reset(now.release());
                was->desc();
            }

            if (count > 1)
                std::this_thread::sleep_for(duration);
        }
    }

    void putStamp() const noexcept
    {
        std::time_t now = std::time(nullptr);
        const std::tm parts = *std::localtime(&now);
        
        char line[64];

        std::strftime(line, sizeof(line), "%m-%d %H:%M:%S", &parts);

        std::cerr << line;
    }

protected:
    const Cfg       &cfg;
};

#endif/*H_FINCORE_MONIT*/
