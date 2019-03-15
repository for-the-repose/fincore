/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

namespace NUtils {
    class TSpan {
    public:
        TSpan() noexcept : TSpan(0, 0) { }

        TSpan(size_t at_, size_t bytes_) noexcept
            : at(at_), bytes(bytes_) { }

        explicit operator bool() const noexcept {
            return bytes > 0;
        }

        size_t after() const noexcept {
            return at + bytes;
        }

        size_t advance(size_t piece) noexcept
        {
            piece = std::min(piece, bytes);

            at      += piece;
            bytes   -= piece;

            return piece;
        }

        bool join(const TSpan &span) noexcept
        {
            if (after() == span.at) {
                bytes += span.bytes;

                return true;
            }

            return false;
        }

        size_t at = 0;
        size_t bytes = 0;
    };

    class TGran : public TSpan {
    public:
        TGran(unsigned page_, const TSpan &span)
            : TSpan(span), page(page_) { }

        size_t pages() const noexcept {
            return (bytes + page - 1) / page;
        }

        size_t paged() const noexcept {
            return page * pages();
        }

        unsigned gran() const noexcept {
            return page;
        }

    protected:
        unsigned    page = 0;
    };

    void swap(TSpan &left, TSpan &right) noexcept
    {
        std::swap(left.at,    right.at);
        std::swap(left.bytes, right.bytes);
    }
}
