/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_FILE
#define H_FINCORE_FILE

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "error.h"
#include "span.h"

namespace OS {
    class Loc {
    public:
        Loc(dev_t dev_ = 0, ino_t ino_ = 0)
            : dev(dev_), ino(ino_) { }

        dev_t   dev     = 0;
        ino_t   ino     = 0;
    };

    enum FType {
        EInvalid    = 0,
        EReg        = 1,
        EDir        = 2,
        EChar       = 3,
        EBlock      = 4,
        EFifo       = 5,
        ELink       = 6,
        ESock       = 7,
        EOther      = 8,
        EAccess     = 9,
    };

    class MemRg : public Utils::Gran {
    public:
        using Gran::Gran;

        operator bool() const noexcept {
            return bytes > 0 && at != 0;
        }

        operator char*() const noexcept {
            return reinterpret_cast<char*>(at);
        }

        operator void*() const noexcept {
            return reinterpret_cast<void*>(at);
        }
    };

    class Mapped {
    public:
        using Span = Utils::Span;

        Mapped() noexcept : ptr(nullptr) { }

        Mapped(const Mapped&) = delete;

        Mapped(Mapped &&mapped) noexcept : Mapped() {
            *this = std::move(mapped);
        }

        Mapped(int fd, const Span &span_) : ptr(nullptr), span(span_)
        {
            ptr = ::mmap(nullptr, span.bytes, PROT_NONE, MAP_SHARED, fd, span.at);

            if (ptr == MAP_FAILED)
                throw Error("failed invoke mmap() on file");
        }

        ~Mapped()
        {
            if (ptr) {
                ::munmap(ptr, span.bytes);

                ptr = nullptr;
            }
        }

        operator bool() const noexcept {
            return ptr != nullptr;
        }

        Mapped& operator=(const Mapped &) = delete;

        Mapped& operator=(Mapped &&mapped) noexcept
        {
            using namespace std;

            swap(ptr, mapped.ptr);
            swap(span, mapped.span);

            return *this;
        }

        operator MemRg() const noexcept
        {
            size_t at = reinterpret_cast<size_t>(ptr);

            return MemRg(getpagesize(), Span(at, span.bytes));
        }

    protected:
        void        *ptr;
        Span        span;
    };

    class File {
    public:
        File() : fd(-1)  { }

        File(const std::string &path) : File()
        {
            fd = ::open(path.data(), O_RDONLY);

            if (fd < 0)
                throw Error("cannot open file");
        }

        ~File() noexcept {
            close();
        }

        File& operator=(const File &file) = delete;

        File& operator=(File &&file)
        {
            close();

            std::swap(fd, file.fd);

            return *this;
        }

        operator bool() const noexcept {
            return fd > -1;
        }

        operator int() const noexcept {
            return fd;
        }

        void close() noexcept
        {
            if (*this) {
                ::close(fd);

                fd = -1;
            }
        }

        size_t size() const throw()
        {
            off_t was = ::lseek(fd, 0, SEEK_CUR);
            off_t size = ::lseek(fd, 0, SEEK_END);

            ::lseek(fd, was, SEEK_SET);

            return size;
        }

        void evict(const Utils::Span &sp) const
        {
            int eno = ::posix_fadvise(fd, sp.at, sp.bytes, POSIX_FADV_DONTNEED);

            if (eno != 0 ) {
                throw Error("failed to invoke fadvise() on file");
            }
        }

        Mapped mmap() const {
            return Mapped(fd, Utils::Span(0, size()));
        }

    private:
        int         fd;
    };

    class Stat {
    public:
        Stat(const std::string &path)
        {
            struct stat st;

            if (lstat(path.c_str(), &st) == 0) {
                feed(st);

            } else if (errno == EACCES) {
                type = EAccess;
            }
        }

        void feed(const struct stat &st) noexcept
        {
            loc = Loc(st.st_dev, st.st_ino);

            links = st.st_nlink;

            if (S_ISREG(st.st_mode)) {
                type = EReg;
            } else if (S_ISDIR(st.st_mode)) {
                type = EDir;
            } else if (S_ISCHR(st.st_mode)) {
                type = EChar;
            } else if (S_ISBLK(st.st_mode)) {
                type = EBlock;
            } else if (S_ISFIFO(st.st_mode)) {
                type = EFifo;
            } else if (S_ISLNK(st.st_mode))  {
                type = ELink;
            } else if (S_ISSOCK(st.st_mode)) {
                type = ESock;
            }
        }

        FType       type    = EInvalid;
        Loc         loc;
        nlink_t     links   = 0;
    };
}

#endif/*H_FINCORE_FILE*/

