/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "error.h"
#include "span.h"

namespace NOs {

    struct TLoc {
        TLoc(dev_t dev_ = 0, ino_t ino_ = 0)
            : Dev(dev_), Ino(ino_) { }

        dev_t Dev = 0;
        ino_t Ino  = 0;
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

    struct TMemRg : public NUtils::TGran {
        using TGran::TGran;

        explicit operator bool() const noexcept {
            return bytes > 0 && at != 0;
        }

        operator char*() const noexcept {
            return reinterpret_cast<char*>(at);
        }

        operator void*() const noexcept {
            return reinterpret_cast<void*>(at);
        }
    };

    class TMapped {
    public:
        using TSpan = NUtils::TSpan;

        TMapped() noexcept { }
        TMapped(const TMapped&) = delete;

        TMapped(TMapped &&mapped) noexcept {
            *this = std::move(mapped);
        }

        TMapped(int fd, const TSpan &span_) : span(span_)
        {
            ptr = ::mmap(nullptr, span.bytes, PROT_READ, MAP_SHARED, fd, span.at);

            if (ptr == MAP_FAILED)
                throw TError("failed invoke mmap() on file");
        }

        ~TMapped()
        {
            if (auto *was = std::exchange(ptr, nullptr)) {
                ::munmap(was, span.bytes);
            }
        }

        explicit operator bool() const { return ptr; }

        void *operator*() const { return ptr; }

        TMapped& operator=(const TMapped &) = delete;

        TMapped& operator=(TMapped &&mapped) noexcept
        {
            std::swap(ptr, mapped.ptr);
            std::swap(span, mapped.span);

            return *this;
        }

        operator TMemRg() const noexcept
        {
            size_t at = reinterpret_cast<size_t>(ptr);

            return TMemRg(getpagesize(), TSpan(at, span.bytes));
        }

    protected:
        void        *ptr = nullptr;
        TSpan       span;
    };

    class TFile {
    public:
        TFile() = default;

        TFile(const std::string &path)
        {
            if ((fd = ::open(path.data(), O_RDONLY)) < 0)
                throw TError("cannot open file");
        }

        ~TFile() noexcept {
            close();
        }

        TFile& operator=(const TFile&) = delete;

        TFile& operator=(TFile && file)
        {
            close();

            std::swap(fd, file.fd);

            return *this;
        }

        explicit operator bool() const noexcept {
            return fd > -1;
        }

        operator int() const noexcept {
            return fd;
        }

        void close() noexcept
        {
            if (*this) ::close(fd), fd = -1;
        }

        size_t size() const noexcept
        {
            off_t was = ::lseek(fd, 0, SEEK_CUR);
            off_t size = ::lseek(fd, 0, SEEK_END);

            ::lseek(fd, was, SEEK_SET);

            return size;
        }

        void evict(const NUtils::TSpan &sp) const
        {
            int eno = ::posix_fadvise(fd, sp.at, sp.bytes, POSIX_FADV_DONTNEED);

            if (eno != 0 ) {
                throw TError("failed to invoke fadvise() on file");
            }
        }

        TMapped mmap() const {
            return TMapped(fd, NUtils::TSpan(0, size()));
        }

    private:
        int fd = -1;
    };

    struct TStat {
        TStat(const std::string &path)
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
            loc = TLoc(st.st_dev, st.st_ino);

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
        TLoc        loc;
        nlink_t     links   = 0;
    };
}
