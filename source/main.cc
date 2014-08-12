/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#include <string>

#include "monit.h"

int do_trace(int argc, char *argv[]);
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
        << endl;
}

