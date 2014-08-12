/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_FILE
#define H_FINCORE_FILE

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "error.h"
#include "span.h"

class File {
public:
    File(const std::string &path)
        : fd(-1), bytes(0), map(nullptr)
    {
        fd = ::open(path.data(), O_RDONLY);

        if (fd < 0)
            throw Error("cannot open file");
    }

    ~File() noexcept
    {
        close();
    }

    void close() noexcept
    {
        if (map != nullptr) {
            ::munmap(map, bytes);

            map = nullptr;
        }

        if (fd > -1) { 
            ::close(fd);

            fd = -1;
        }
    }

    explicit operator int() const noexcept
    {
        return fd;
    }

    size_t size() const throw()
    {
        off_t was = ::lseek(fd, 0, SEEK_CUR);
        off_t size = ::lseek(fd, 0, SEEK_END);

        ::lseek(fd, was, SEEK_SET);

        return size;
    }

    void evict(const Stats::Span &sp) const
    {
        int eno = ::posix_fadvise(fd, sp.at, sp.bytes, POSIX_FADV_DONTNEED);

        if (eno != 0 ) {
            throw Error("failed to invoke fadvise() on file");
        }
    }

    void* mmap()
    {
        if (map == nullptr) {
            bytes = size();

            map = ::mmap(nullptr, bytes, PROT_NONE, MAP_SHARED, fd, 0);

            if (map == nullptr)
                throw Error("failed invoke mmap() on file");
        }

        return map;
    }

private:
    int         fd;
    size_t      bytes;
    void        *map;
};

#endif/*H_FINCORE_FILE*/

