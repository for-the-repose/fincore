/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_SPAN
#define H_FINCORE_SPAN

namespace Utils {
    class Span {
    public:
        Span() noexcept : Span(0, 0) { }

        Span(size_t at_, size_t bytes_) noexcept
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

        bool join(const Span &span) noexcept
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

    class Gran : public Span {
    public:
        Gran(unsigned page_, const Span &span)
            : Span(span), page(page_) { }

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

    void swap(Span &left, Span &right) noexcept
    {
        std::swap(left.at,    right.at);
        std::swap(left.bytes, right.bytes);
    }
}

#endif/*H_FINCORE_SPAN*/
