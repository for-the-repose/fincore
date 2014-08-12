/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_SPAN
#define H_FINCORE_SPAN

namespace Stats {
    class Span {
    public:
        Span(size_t at_, size_t bytes_)
            : at(at_), bytes(bytes_)
        {

        }

        operator bool() const noexcept
        {
            return bytes > 0;
        }

        size_t after() const noexcept
        {
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

        size_t  at;
        size_t  bytes;
    };
}

#endif/*H_FINCORE_SPAN*/
