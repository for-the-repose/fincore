/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <exception>
#include <string>

class TError : public std::exception {
public:
    TError(const std::string &desc_) : Desc(desc_) {  }

    const char* what() const noexcept { return Desc.data(); }

    std::string Desc;
};

