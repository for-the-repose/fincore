#pragma once /*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

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

    enum ENode {
        None    	= 0,
        File        = 1,
        Dir         = 2,
        Char        = 3,
        Block       = 4,
        Fifo        = 5,
        Link        = 6,
        Sock        = 7,
        Other       = 8,
        Access      = 9,
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

            return { size_t(getpagesize()), TSpan(at, span.bytes) };
        }

    protected:
        void        *ptr = nullptr;
        TSpan       span;
    };

    class TFile {
    public:
        TFile() = default;

        TFile(const std::string &path, bool direct = false)
        {
            int flags = O_RDONLY | (direct ? O_DIRECT : 0);

            if ((fd = ::open(path.data(), flags)) < 0)
                throw TError("cannot open file");
        }

        ~TFile() noexcept { Close(); }

        TFile& operator=(const TFile&) = delete;

        TFile& operator=(TFile && file)
        {
            Close();

            std::swap(fd, file.fd);

            return *this;
        }

        explicit operator bool() const noexcept { return fd > -1; }

        operator int() const noexcept { return fd; }

        void Close() noexcept
        {
            if (*this) ::close(std::exchange(fd, -1));
        }

        size_t Size() const noexcept
        {
            off_t was = ::lseek(fd, 0, SEEK_CUR);
            off_t size = ::lseek(fd, 0, SEEK_END);

            ::lseek(fd, was, SEEK_SET);

            return size;
        }

        void Evict(const NUtils::TSpan &sp) const
        {
            int eno = ::posix_fadvise(fd, sp.at, sp.bytes, POSIX_FADV_DONTNEED);

            if (eno != 0 ) {
                throw TError("failed to invoke fadvise() on file");
            }
        }

        TMapped MMap() const { return { fd, NUtils::TSpan(0, Size()) }; }

    private:
        int fd = -1;
    };

    struct TStat {
		TStat(const TFile &file)
		{
            struct stat st;

            Set(fstat(file, &st), st);
		}

        TStat(const std::string &path)
        {
            struct stat st;

            Set(lstat(path.c_str(), &st), st);
        }

        void Set(int rv, const struct stat &st) noexcept
        {
			if (rv == 0) {
				Loc = TLoc(st.st_dev, st.st_ino);

				Links = st.st_nlink;
                Bytes = st.st_size;

				if (S_ISREG(st.st_mode)) {
					Type = ENode::File;
				} else if (S_ISDIR(st.st_mode)) {
					Type = ENode::Dir;
				} else if (S_ISCHR(st.st_mode)) {
					Type = ENode::Char;
				} else if (S_ISBLK(st.st_mode)) {
					Type = ENode::Block;
				} else if (S_ISFIFO(st.st_mode)) {
					Type = ENode::Fifo;
				} else if (S_ISLNK(st.st_mode))  {
					Type = ENode::Link;
				} else if (S_ISSOCK(st.st_mode)) {
					Type = ENode::Sock;
				}
			} else if (errno == EACCES) {
                Type = ENode::Access;
            }
        }

        ENode       Type    = ENode::None;
        TLoc        Loc;
        uint32_t    Links   = 0;
        uint64_t    Bytes   = 0;
    };
}
