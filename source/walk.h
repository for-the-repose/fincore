/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_WALK
#define H_FINCORE_WALK

#include <dirent.h>
#include <sys/types.h>

#include <list>

namespace Utils {
    namespace Dir {
        class Ref {
        public:
            Ref() : type(OS::EInvalid), depth(0) { }

            Ref(Ref &&ref)
                : type(ref.type), depth(ref.depth)
            {
                name = std::move(ref.name);
            }

            Ref(OS::FType type_, unsigned depth_, std::string && name_)
                : type(type_), depth(depth_), name(name_)
            {
                assert(type != OS::EInvalid);
            }

            operator bool() const noexcept
            {
                return type != OS::EInvalid;
            }

            bool IsAbove(const Ref &ref) const noexcept
            {
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

            Path(const std::string &base) : Path()
            {
                add(base);
            }

            Path& add(const Ref &ref)
            {
                return add(ref.name);
            }

            Path& add(const std::string &name)
            {
                if (level++ > 0)
                    path.append(1, '/');

                path.append(name);

                return *this;
            }

            operator const std::string&() const noexcept
            {
                return path;
            }

            operator std::string&() && noexcept
            {
                return path;
            }

        private:
            size_t          level;
            std::string     path;
        };

        class Iter {
        public:
            Iter() : stream(nullptr) { }

            Iter(const Iter&) = delete;

            Iter(Iter &&iter)
            {
                std::swap(stream, iter.stream);
            }

            Iter(const std::string &path) : Iter()
            {
                stream = opendir(path.c_str());

                if (stream == nullptr)
                    throw Error("Cannot open directory");
            }

            ~Iter() noexcept
            {
                close();
            }

            operator bool() const noexcept
            {
                return stream != nullptr;
            }

            void operator =(Iter &&iter) noexcept
            {
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

        class Walk {
        public:
            Walk(const std::string &path)
            {
                deep(path);
            }

            operator bool() const noexcept
            {
                return !stack.empty();
            }

            Ref next()
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
    }
}

#endif/*H_FINCORE_WALK*/
