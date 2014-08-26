/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_WALK
#define H_FINCORE_WALK

#include <dirent.h>
#include <sys/types.h>

#include <sstream>
#include <list>
#include "span.h"

namespace Utils {
    namespace Dir {
        class Ref {
        public:
            Ref() : type(OS::EInvalid), depth(0) { }

            Ref(Ref &&ref) : type(ref.type), depth(ref.depth) {
                name = std::move(ref.name);
            }

            Ref(OS::FType type_, unsigned depth_, std::string && name_)
                : type(type_), depth(depth_), name(name_)
            {
                assert(type != OS::EInvalid);
            }

            operator bool() const noexcept {
                return type != OS::EInvalid;
            }

            bool IsAbove(const Ref &ref) const noexcept {
                return depth < ref.depth;
            }

            OS::FType       type;
            unsigned        depth;
            std::string     name;
        };

        void swap(Ref &left, Ref &right)
        {
            std::swap(left.type, right.type);
            std::swap(left.depth, right.depth);
            std::swap(left.name, right.name);
        }

        class Path {
        public:
            Path() : level(0) { }

            Path(const std::string &base) : Path() {
                if (!base.empty()) add(base);
            }

            Path& add(const Ref &ref) {
                return add(ref.name);
            }

            Path& add(const std::string &name)
            {
                if (level++ > 0)
                    path.append(1, '/');

                path.append(name);

                return *this;
            }

            operator const std::string&() const noexcept {
                return path;
            }

            operator std::string&() && noexcept {
                return path;
            }

        private:
            size_t          level;
            std::string     path;
        };

        class Enum {
        public:
            virtual ~Enum() noexcept { }
            virtual operator bool() const noexcept = 0;
            virtual Ref next() = 0;
        };

        class Iter {
        public:
            Iter() : stream(nullptr) { }

            Iter(const Iter&) = delete;

            Iter(Iter &&iter) {
                std::swap(stream, iter.stream);
            }

            Iter(const std::string &path) : Iter()
            {
                stream = opendir(path.c_str());

                if (stream == nullptr)
                    throw Error("Cannot open directory");
            }

            ~Iter() noexcept {
                close();
            }

            operator bool() const noexcept {
                return stream != nullptr;
            }

            void operator =(Iter &&iter) noexcept {
                std::swap(stream, iter.stream);
            }

            Ref next()
            {
                while(*this) {
                    struct dirent *entry = readdir(stream);

                    if (entry != nullptr) {
                        auto name = std::string(entry->d_name);

                        if (name == ".." || name == ".")
                            continue;

                        OS::FType type = OS::EOther;

                        if (entry->d_type == DT_REG) {
                            type = OS::EReg;

                        } else if (entry->d_type == DT_DIR) {
                            type = OS::EDir;

                        } else if (entry->d_type == DT_LNK) {
                            type = OS::ELink;

                        } else if (entry->d_type == DT_UNKNOWN) {

                        }

                        return Ref(type, 0, std::move(name));

                    } else
                        close();
                }

                return Ref();
            }

        protected:
            void close() noexcept
            {
                if (stream) {
                    closedir(stream);

                    stream = nullptr;
                }
            }

            DIR     *stream;
        };

        class Level {
        public:
            Level(const std::string &name_) : name(name_) { }

            void open(const std::string &path)
            {
                iter = Iter(path);
            }

            std::string name;
            Iter        iter;
        };

        class Walk : public Enum {
        public:
            Walk(const std::string &path) {
                deep(path);
            }

            operator bool() const noexcept override {
                return !stack.empty();
            }

            Ref next() override
            {
                while (!stack.empty()) {
                    Level &level = stack.back();

                    Ref label = level.iter.next();
                    auto depth = stack.size();

                    if (!label) {
                        stack.pop_back();

                    } else if (label.type == OS::EDir) {
                        try {
                            deep(label.name);

                            return Ref(OS::EDir, depth, trace(false));

                        } catch (Error &error) {

                            return Ref(OS::EAccess, depth, trace(false));
                        }

                    } else {
                        auto path = trace(false).add(label);

                        return Ref(label.type, depth, path);
                    }
                }

                return Ref();
            }

        protected:
            void deep(const std::string &name)
            {
                stack.emplace_back(name);

                stack.back().open(trace(true));
            }

            Path trace(bool full) const noexcept
            {
                Path path;

                if (!stack.empty()) {
                    auto it = std::next(stack.begin(), full ? 0 : 1);

                    for (; it != stack.end(); it++) path.add(it->name);
                }

                return path;
            }

        private:
            using Stack = std::list<Level>;

            Stack       stack;
        };

        class List : public Enum {
        public:
            List(std::istream &in_) : in(in_)
            {

            }

            operator bool() const noexcept override {
                return in || !refs.empty();
            }

            Ref next() override
            {
                while (*this) {
                    if (refs.empty()) {
                        std::string line;

                        if (std::getline(in, line)) process(line);
                    }

                    if (!refs.empty()) {
                        Ref ref = std::move(refs.front());

                        refs.pop_front();

                        return std::move(ref);
                    }
                }

                return Ref();
            }

        protected:
            class Slice : public Utils::Span {
            public:
                using Utils::Span::Span;

                operator size_t() const noexcept {
                    return at;
                }
            };

            void examine(size_t depth) noexcept
            {
                OS::Stat info(stack);

                if (info.type != OS::EDir) {

                    edge = depth;
                }

                if (info.type != OS::EInvalid) {
                    using namespace std;

                    refs.emplace_back(info.type, depth, move(string(stack)));
                }
            }

            void process(const std::string &path) noexcept
            {
                if (!check(path)) {
                    /* TODO: collect stats  */

                } else {
                    Slice on, to;

                    for (size_t depth = 0; ; depth++) {
                        on = forward(stack, on.after());
                        to = forward(path, to.after());

                        if (!same(on, to, path)) {
                            edge = -1;

                            stack.resize((size_t)on - (on ? 1 : 0));

                            for (; to; depth++) {
                                extend(path, to);
                                examine(depth);

                                to = forward(path, to.after());
                            }

                            return;

                        } else if (depth > edge) {
                            /* TODO: collect stats */

                            return;
                        }
                    }
                }
            }

            bool check(const std::string &path) noexcept
            {
                if (path.size() > 0) {
                    const bool absolute = (path[0] == '/');

                    if (first) {
                        relative = !absolute;

                        first = false;
                    }

                    return (relative == !absolute);
                }

                return false;
            }

            static Slice forward(const std::string &path, size_t at) noexcept
            {
                while (true) {
                    const size_t end = path.find('/', at);

                    if (end == path.npos) {
                        return Slice(at, path.size() - at);

                    } else if (end == at) {
                        at++;

                    } else {
                        return Slice(at, end - at);
                    }
                }
            }

            void extend(const std::string &path, const Slice &on) noexcept
            {
                if (!relative || stack.size() > 0) stack.append(1, '/');

                stack.append(path.substr(on, on.bytes));
            }

            bool same(const Slice &on, const Slice &to, const std::string &by)
            {
                return stack.compare(on, on.bytes, by, to, to.bytes) == 0;
            }

            using Keep = std::list<Ref>;

            bool            first       = true;
            bool            relative    = false;
            size_t          edge        = -1;
            std::istream    &in;
            std::string     stack;
            Keep            refs;
        };
    }
}

#endif/*H_FINCORE_WALK*/
