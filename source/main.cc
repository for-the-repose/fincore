/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#include <string>

#include "monit.h"

void usage() noexcept;


int main(int argc, char *argv[])
{
	extern char *optarg;
	
	std::string path;
	Monit::Cfg 	cfg;

	while (true) {
		static const char opts[] = "f:d:c:r:h";

		const int opt = getopt(argc, argv, opts);

		if (opt < 0) break;

		if (opt == 'h') {
			usage();

			return 0;

		} else if (opt == 'f') {
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
	std::cerr << "fincore -f PATH [ -c COUNT] [-d DELAY]" << std::endl;
}

