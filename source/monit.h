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
		unsigned 	delay	= 0;
		size_t 		count	= 1;
		float		thresh	= 0.1;
	};

	Monit(const Cfg &cfg_) : cfg(cfg_) { }

	void Do(std::string &path)
	{
		Stats::Bands aggr(48);
		Probe probe;

		const std::chrono::seconds duration(cfg.delay);

		Stats::FeedRef	was;

		for(unsigned count = cfg.count; count > 0; count--) {
			auto now = probe(path, aggr);

			if (!was.get() || was->diff(*now, cfg.thresh)) {
				putStamp();

				was = now;
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

		std::strftime(line, sizeof(line), "%Y-%m-%d %H:%M:%S", &parts);

		std::cerr << line;
	}

protected:
	const Cfg		&cfg;
};

#endif/*H_FINCORE_MONIT*/
