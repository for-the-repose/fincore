/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_ERROR
#define H_FINCORE_ERROR

#include <exception>
#include <string>

class Error : public std::exception {
public:
	Error(const std::string &desc_) : desc(desc_)
	{

	}

	const char* what() const noexcept
	{
		return desc.data();
	}

	std::string desc;
};



#endif/*H_FINCORE_ERROR*/

