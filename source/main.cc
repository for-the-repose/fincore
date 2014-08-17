/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#include <string>

#include "monit.h"
#include "file.h"
#include "top.h"

int do_trace(int argc, char *argv[]);
int do_evict(int argc, char *argv[]);
int do_stats(int argc, char *argv[]);
void usage() noexcept;


int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();

        return 0;

    } else {
        std::string mod(argv[1]);

        if (mod == "trace") {
            return do_trace(argc--, argv++);

        } else if (mod == "evict") {
            return do_evict(argc--, argv++);

        } else if (mod == "stats") {
            return do_stats(argc--, argv++);

        } else {
            std::cerr << "unknown mode " << mod << std::endl;

            return 1;
        }
    }

    return 0;
}


int do_trace(int argc, char *argv[])
{
    extern char *optarg;
    
    std::string path;
    Monit::Cfg  cfg;

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
        Monit(cfg).Do(path);
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
        OS::File file(path);

        const Utils::Span all(0, file.size());

        file.evict(all);
    }

    return 0;
}


int do_stats(int argc, char *argv[])
{
    extern char *optarg;

    std::string path;
    Top::Cfg    cfg;

    while (true) {
        static const char opts[] = "f:d:r:l:zs";

        const int opt = getopt(argc, argv, opts);

        if (opt < 0) break;

        if (opt == 'f') {
            path = optarg;

        } else if (opt == 'd') {
            cfg.edge = std::stoull(optarg);

        } else if (opt == 'z') {
            cfg.zeroes = true;

        } else if (opt == 's') {
            cfg.summary = true;

        } else if (opt == 'l') {
            cfg.limit = std::stoull(optarg);

        } else if (opt == 'r') {
            const std::string rname(optarg);

            if (rname == "none") {
                cfg.reduct = Top::Cfg::REDUCT_NONE;

            } else if (rname == "top") {
                cfg.reduct = Top::Cfg::REDUCT_TOP;

            } else {
                std::cerr << "unknown reductor " << rname << std::endl;

                return 1;
            }
        }
    }

    if (path.empty()) {
        std::cerr << "path to directory is not given" << std::endl;

    } else {
        Top(cfg).Do(path);
    }

    return 0;
}


void usage() noexcept
{
    using namespace std;

    cerr
        << "fincore mode [ ARGS ] ..."
        << endl
        << endl << " Options for trace"
        << endl << "   -f path    path to file for tracing"
        << endl << "   -c count   how many snaps make"
        << endl << "   -d gran    time granulation, secs"
        << endl << "   -r float   refresh changes threshold"
        << endl
        << endl << " Options for evict"
        << endl << "   -f path    path to file for evicting"
        << endl
        << endl << " Options for stats"
        << endl << "   -f path    path to directory for stats"
        << endl << "   -d depth   depth detalization limit"
        << endl << "   -z         show entries with zero usage"
        << endl << "   -r kind    type of reduction: none, top"
        << endl << "   -l items   items limit for reduction"
        << endl << "   -s         collect root summary stats"
        << endl;
}

