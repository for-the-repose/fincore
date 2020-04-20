/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#include <string>
#include <sys/mman.h>

#include "monit.h"
#include "file.h"
#include "top.h"
#include "touch.h"
#include "write.h"


int do_trace(int argc, char *argv[]);
int do_evict(int argc, char *argv[]);
int do_stats(int argc, char *argv[]);
int do_lock(int args, char *argv[]);
void usage() noexcept;


int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();

        return 0;

    } else {
        std::string mod(argv[1]);

        try {
            if (mod == "trace") {
                return do_trace(argc--, argv++);
            } else if (mod == "evict") {
                return do_evict(argc--, argv++);
            } else if (mod == "stats") {
                return do_stats(argc--, argv++);
            } else if (mod == "lock") {
                return do_lock(argc--, argv++);
            } else if (mod == "read") {
                return TMod_Read().Handle(argc--, argv++);
            } else if (mod == "write") {
                return TMod_Write().Handle(argc--, argv++);
            } else {
                std::cerr << "unknown mode " << mod << std::endl;

                return 1;
            }
        } catch (TError &err) {
            std::cerr << err.what() << std::endl;

            return 3;
        }
    }

    return 0;
}


int do_trace(int argc, char *argv[])
{
    extern char *optarg;
    
    std::string path;
    TMonit::TCfg  cfg;

    while (true) {
        static const char opts[] = "f:d:c:r:";

        const int opt = getopt(argc, argv, opts);

        if (opt < 0) break;

        if (opt == 'f') {
            path = optarg;
        } else if (opt == 'd') {
            cfg.delay = std::stoull(optarg);
        } else if (opt == 'c') {
            cfg.count = std::stoull(optarg);
        } else if (opt == 'r') {
            cfg.thresh = std::stod(optarg);
        }
    }

    if (path.empty()) {
        std::cerr << "path to file is not given" << std::endl;

        return 1;

    } else {
        TMonit(cfg).Do(path);
    }

    return 0;
}


int do_evict(int argc, char *argv[])
{
    extern char *optarg;

    std::string path;

    while (true) {
        static const char opts[] = "f:";

        const int opt = getopt(argc, argv, opts);

        if (opt < 0) break;

        if (opt == 'f') {
            path = optarg;
        }
    }

    if (path.empty()) {
        std::cerr << "path to file is not given" << std::endl;

    } else {
        NOs::TFile file(path);

        const NUtils::TSpan all(0, file.Size());

        file.Evict(all);
    }

    return 0;
}


int do_stats(int argc, char *argv[])
{
    extern char *optarg;

    std::string path;
    bool        input = false;
    TTop::TCfg  cfg;

    while (true) {
        static const char opts[] = "f:d:r:l:c:zsi";

        const int opt = getopt(argc, argv, opts);

        if (opt < 0) break;

        if (opt == 'f') {
            path = optarg;
        } else if (opt == 'i') {
            input = true;
        } else if (opt == 'd') {
            cfg.edge = std::stoull(optarg);
        } else if (opt == 'z') {
            cfg.zeroes = true;
        } else if (opt == 's') {
            cfg.summary = true;
        } else if (opt == 'l') {
            cfg.limit = std::stoull(optarg);
        } else if (opt == 'c') {
            cfg.raito = std::stod(optarg);
        } else if (opt == 'r') {
            const std::string rname(optarg);

            if (rname == "none") {
                cfg.reduct = TTop::TCfg::REDUCT_NONE;
            } else if (rname == "top") {
                cfg.reduct = TTop::TCfg::REDUCT_TOP;
            } else {
                std::cerr << "unknown reductor " << rname << std::endl;

                return 1;
            }
        }
    }

    if (!path.empty() && input) {
        std::cerr << "only one of -f or -i allowed" << std::endl;
    } else if (!path.empty()){
        TTop(cfg.validate()).Do(path);
    } else if (input) {
        TTop(cfg.validate()).Do(std::cin);
    } else {
        std::cerr << "path to directory is not given" << std::endl;
    }

    return 0;
}


int do_lock(int argc, char *argv[])
{
    extern char *optarg;

    std::string path;
    uint64_t seconds = 0;

    while (true) {
        static const char opts[] = "f:s:";

        const int opt = getopt(argc, argv, opts);

        if (opt < 0) break;

        if (opt == 'f') {
            path = optarg;
        } else if (opt == 's') {
            seconds = std::stoull(optarg);
        }
    }

    if (path.empty()) {
        std::cerr << "path to file is not given" << std::endl;
    } else {
        NOs::TFile file(path);
        auto map = file.MMap();
        auto bytes = NOs::TMemRg(map).bytes;

        if (mlock(*map, bytes)) {
            std::cerr << "Cannot lock memory, errno " << errno << "\n";
        } else {
            std::cerr << "Locked " << bytes << " bytes of " << path << "\n";

            sleep(seconds);

            std::cerr << "Unlocking memory after " << seconds << " seconds\n";
        }
    }

    return 0;
}


void usage() noexcept
{
    std::cerr
        << "fincore mode [ ARGS ] ..."
        << "\n\n Mode `trace`, show compact single file cache map"
        << "\n   -f path    Path to file for tracing"
        << "\n   -c count   How many snaps make"
        << "\n   -d gran    Time granulation, secs"
        << "\n   -r float   Refresh changes threshold"
        << "\n   -s sampl   Minimal samples bands"
        << "\n\n Mode `evict`, try to evicts file data from memory"
        << "\n   -f path    Path to file for evicting"
        << "\n\n Mode `stats`, collects files cache raito"
        << "\n   -f path    Path to directory for stats"
        << "\n   -i         Read path names from stdin"
        << "\n   -d depth   Depth detalization limit"
        << "\n   -z         Show entries with zero usage"
        << "\n   -r kind    Type of reduction: none, top"
        << "\n   -l items   Items limit for reduction"
        << "\n   -s         Collect root summary stats"
        << "\n   -c raito   Cache filter raito for aggr"
        << "\n\n Mope `lock`, locks file for a while"
        << "\n   -f path    Path to file for locking in memory"
        << "\n   -s seconds How long to keep memory locked"
        << "\n\n Mode `read`, generates IO read load on a file"
        << "\n   -f path    Path to real file for read from"
        << "\n   -b bytes   Read granularity in bytes"
        << "\n   -r msecs   Read period in milliseconds (ms)"
        << "\n   -c cycles  Number of block reads to perform"
        << "\n   -m mode    Mode: seq - sequential, rnd - random"
        << "\n\n Mode `write`, generates IO write load to a file"
        << "\n   -f path    Path to file, will be overwritten"
        << "\n   -b bytes   Write granularity in bytes"
        << "\n   -s bytes   Total file size to produce, rounded"
        << "\n   -r msecs   Write period in milliseconds (ms)"
        << "\n   -c cycles  Number of block writes to perform"
        << "\n   -m mode    Mode: seq - sequential, rnd - random"
        << "\n   -u skip    sync fd each skip write cycles"
        << std::endl;
}

