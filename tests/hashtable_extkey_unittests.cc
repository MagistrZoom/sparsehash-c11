#include "gtest/gtest.h"

#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>
#include <sparsehash/wrapped_dense_hash_map>
#include <sparsehash/wrapped_dense_hash_set>

#include <functional>
#include <string>

using google::dense_hash_map;
using google::dense_hash_set;
using sparsehash::wrapped_dense_hash_set;
using sparsehash::wrapped_dense_hash_map;

namespace extkey_tests {

class A
{
    std::string str_;
public:
    static int ctor;
    static int copy_ctor;
    static int copy_assign;
    static int move_ctor;
    static int move_assign;

    static int str_ctor;
    static int str_copy_ctor;
    static int str_copy_assign;
    static int str_move_ctor;
    static int str_move_assign;

    static void reset()
    {
        ctor = 0;
        copy_ctor = 0;
        copy_assign = 0;
        move_ctor = 0;
        move_assign = 0;
        str_ctor = 0;
        str_copy_ctor = 0;
        str_copy_assign = 0;
        str_move_ctor = 0;
        str_move_assign = 0;
    }

    A() { ++ctor; ++str_ctor; }
    A(const std::string& s): str_(s) { ++ctor; ++str_copy_ctor; }
    A(std::string&& s): str_(std::move(s)) { ++ctor; ++str_move_ctor; }
    A(const char* s): str_(s) { ++ctor; ++str_ctor; }

    A(const A& a): str_(a.str_) { ++copy_ctor; ++str_copy_ctor; }
    A& operator=(const A& a) { str_ = a.str_; ++copy_assign; ++str_copy_assign; return *this; }

    A(A&& a): str_(std::move(a.str_)) { ++move_ctor; ++str_move_ctor; }
    A& operator=(A&& a) { str_ = std::move(a.str_); ++move_assign; ++str_move_assign; return *this; }

    const std::string& get_str() const { return str_; }

    friend bool operator==(const A& lhs, const A& rhs) { return lhs.str_ == rhs.str_; }
    friend bool operator!=(const A& lhs, const A& rhs) { return lhs.str_ == rhs.str_; }
    friend bool operator==(const A& lhs, const std::string& rhs) { return lhs.str_ == rhs; }
    friend bool operator!=(const A& lhs, const std::string& rhs) { return lhs.str_ == rhs; }
    friend bool operator==(const std::string& lhs, const A& rhs) { return lhs == rhs.str_; }
    friend bool operator!=(const std::string& lhs, const A& rhs) { return lhs == rhs.str_; }
};

int A::ctor = 0;
int A::copy_ctor = 0;
int A::copy_assign = 0;
int A::move_ctor = 0;
int A::move_assign = 0;
int A::str_ctor = 0;
int A::str_copy_ctor = 0;
int A::str_copy_assign = 0;
int A::str_move_ctor = 0;
int A::str_move_assign = 0;

class B
{
    std::string str_;
public:
    B(const std::string& str): str_(str) {}

    const std::string& get_string() const& { return str_; }
    const std::string& get_string() & { return str_; }
    std::string&& get_string() && { return std::move(str_); }
};

struct BtoA
{
    std::tuple<const std::string&> operator() (const B& b) const {
        return std::forward_as_tuple(b.get_string());
    }
    std::tuple<std::string&&> operator() (B&& b) const {
        return std::forward_as_tuple(std::move(b).get_string());
    }
};

struct HashA
    : public std::hash<std::string>
{
    size_t operator() (const A& a) const { return std::hash<std::string>::operator() (a.get_str()); }
    size_t operator() (const std::string& s) const { return std::hash<std::string>::operator() (s); }
    size_t operator() (const B& b) const { return std::hash<std::string>::operator() (b.get_string()); }
};

struct EqualA
{
    bool operator() (const A& lhs, const A& rhs) const { return lhs == rhs; }
    bool operator() (const A& lhs, const std::string& rhs) const { return lhs == rhs; }
    bool operator() (const std::string& lhs, const A& rhs) const { return lhs == rhs; }
    bool operator() (const A& lhs, const B& rhs) const { return lhs == rhs.get_string(); }
    bool operator() (const B& lhs, const A& rhs) const { return lhs.get_string() == rhs; }
};

template <class K, class Container>
struct container_key_accepts
    : public std::integral_constant<bool, Container::template accept_as_key<K>::value>
{};

template <class K, class Container>
struct container_key_constructible
    : public std::integral_constant<bool, Container::template key_constructible_trait<K>::simple>
{};
template <class K, class Container>
struct container_key_transformable
    : public std::integral_constant<bool, Container::template key_constructible_trait<K>::helper>
{};

typedef dense_hash_set<A, HashA, EqualA, google::libc_allocator_with_realloc<A>, BtoA> Set;
typedef dense_hash_set<A, HashA, EqualA> SetLookup;
typedef dense_hash_map<A, int, HashA, EqualA, google::libc_allocator_with_realloc<std::pair<const A, int>>, BtoA> Map;
typedef dense_hash_map<A, int, HashA, EqualA> MapLookup;
typedef wrapped_dense_hash_set<A, HashA, EqualA, BtoA> WrappedSet;
typedef wrapped_dense_hash_set<A, HashA, EqualA> WrappedSetLookup;
typedef wrapped_dense_hash_map<A, int, HashA, EqualA, BtoA> WrappedMap;
typedef wrapped_dense_hash_map<A, int, HashA, EqualA> WrappedMapLookup;

}

using namespace extkey_tests;

TEST(DenseHashSetLookupExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, SetLookup> AcceptsA;
    typedef container_key_accepts<B, SetLookup> AcceptsB;
    typedef container_key_accepts<std::string, SetLookup> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(DenseHashSetLookupExtKeyTest, Find)
{
    SetLookup set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = set.emplace("Hello").first;
    auto comma_it = set.emplace(",").first;
    auto world_it = set.emplace("world").first;
    set.emplace("!");

    A::reset();
    auto hello_it_f = set.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = set.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = set.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = set.find("!");
    EXPECT_FALSE(it == set.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(DenseHashSetLookupExtKeyTest, Count)
{
    SetLookup set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, set.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, set.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, set.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(DenseHashSetLookupExtKeyTest, EqualRange)
{
    SetLookup set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = set.emplace("Hello").first;
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    auto range = set.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = set.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == set.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(DenseHashSetLookupExtKeyTest, Erase)
{
    SetLookup set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, set.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, set.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::copy_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "erase by acceptable key substitute";
}

TEST(DenseHashSetExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, Set> AcceptsA;
    typedef container_key_accepts<B, Set> AcceptsB;
    typedef container_key_accepts<std::string, Set> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(DenseHashSetExtKeyTest, Find)
{
    Set set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = set.emplace("Hello").first;
    auto comma_it = set.emplace(",").first;
    auto world_it = set.emplace("world").first;
    set.emplace("!");

    A::reset();
    auto hello_it_f = set.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = set.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = set.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = set.find("!");
    EXPECT_FALSE(it == set.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(DenseHashSetExtKeyTest, Count)
{
    Set set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, set.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, set.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, set.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(DenseHashSetExtKeyTest, EqualRange)
{
    Set set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = set.emplace("Hello").first;
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    auto range = set.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = set.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == set.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(DenseHashSetExtKeyTest, Erase)
{
    Set set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, set.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, set.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::copy_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "erase by acceptable key substitute";
}

TEST(DenseHashSetExtKeyTest, Emplace)
{
    Set set;
    set.set_empty_key(A{"<empty>"});
    set.set_deleted_key(A{"<deleted>"});

    set.emplace(B{"Hello"});
    set.emplace(B{","});
    set.emplace(B{"world"});

    A::reset();
    EXPECT_FALSE(set.emplace(B{"Hello"}).second) << "emplace of existing element";
    ASSERT_EQ(0, A::ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of existing element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::move_assign) << "emplace of existing element";

    A::reset();
    set.emplace(std::string{"New one"});
    ASSERT_EQ(1, A::ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of new element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::move_assign) << "emplace of new element";
    ASSERT_EQ(0, A::str_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of new element";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of new element";

    A::reset();
    set.emplace(B{"New one Second"});
    ASSERT_EQ(1, A::ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of new element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::move_assign) << "emplace of new element";
    ASSERT_EQ(0, A::str_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of new element";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of new element";

    A::reset();
    set.emplace("New one");
    ASSERT_EQ(1, A::ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of new constructed element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::move_assign) << "emplace of new constructed element";
    ASSERT_EQ(1, A::str_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_move_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of new constructed element";
}

TEST(DenseHashMapLookupExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, MapLookup> AcceptsA;
    typedef container_key_accepts<B, MapLookup> AcceptsB;
    typedef container_key_accepts<std::string, MapLookup> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(DenseHashMapLookupExtKeyTest, Find)
{
    MapLookup map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = map.emplace("Hello", 0).first;
    auto comma_it = map.emplace(",", 1).first;
    auto world_it = map.emplace("world", 2).first;
    map.emplace("!", 3);

    A::reset();
    auto hello_it_f = map.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = map.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = map.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = map.find("!");
    EXPECT_FALSE(it == map.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(DenseHashMapLookupExtKeyTest, Count)
{
    MapLookup map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, map.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, map.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, map.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(DenseHashMapLookupExtKeyTest, EqualRange)
{
    MapLookup map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = map.emplace("Hello", 0).first;
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    auto range = map.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = map.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == map.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(DenseHashMapLookupExtKeyTest, Erase)
{
    MapLookup map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, map.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, map.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::copy_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "erase by acceptable key substitute";
}

TEST(DenseHashMapExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, Map> AcceptsA;
    typedef container_key_accepts<B, Map> AcceptsB;
    typedef container_key_accepts<std::string, Map> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
    typedef container_key_constructible<std::string, Map> StringConstructible;
    typedef container_key_transformable<std::string, Map> StringTransformable;
    typedef container_key_constructible<B, Map> BConstructible;
    typedef container_key_transformable<B, Map> BTransformable;
    EXPECT_TRUE(StringConstructible::value);
    EXPECT_FALSE(StringTransformable::value);
    EXPECT_FALSE(BConstructible::value);
    EXPECT_TRUE(BTransformable::value);
}

TEST(DenseHashMapExtKeyTest, Find)
{
    Map map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = map.emplace("Hello", 0).first;
    auto comma_it = map.emplace(",", 1).first;
    auto world_it = map.emplace("world", 2).first;
    map.emplace("!", 3);

    A::reset();
    auto hello_it_f = map.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = map.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = map.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = map.find("!");
    EXPECT_FALSE(it == map.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(DenseHashMapExtKeyTest, Count)
{
    Map map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, map.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, map.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, map.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(DenseHashMapExtKeyTest, EqualRange)
{
    Map map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    auto hello_it = map.emplace("Hello", 0).first;
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    auto range = map.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = map.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == map.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(DenseHashMapExtKeyTest, Erase)
{
    Map map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, map.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, map.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::copy_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "erase by acceptable key substitute";
}

TEST(DenseHashMapExtKeyTest, Emplace)
{
    Map map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    // emplace key substitute, from which key cannot be constructed
    // without helper
    A::reset();
    auto res = map.emplace(B{"Hello"}, 0);
    ASSERT_EQ(1, A::ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::copy_assign) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::move_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::move_assign) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of indirect key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of indirect key substitute";

    // emplace key substitue, from which key can be constructed
    A::reset();
    map.emplace(std::string{","}, 1);
    ASSERT_EQ(1, A::ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::copy_assign) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::move_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::move_assign) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of direct key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of direct key substitute";

    // emplace value, from which key can be constructed, but which
    // cannot be used as a key substitute
    A::reset();
    map.emplace("world", 2);
    ASSERT_EQ(1, A::ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::copy_assign) << "emplace of a value, convertible to key";
    ASSERT_EQ(1, A::move_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::move_assign) << "emplace of a value, convertible to key";
    ASSERT_EQ(1, A::str_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of a value, convertible to key";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of a value, convertible to key";


    A::reset();
    EXPECT_FALSE(map.emplace(B{"Hello"}, 3).second) << "emplace of existing element";
    EXPECT_EQ(0, res.first->second) << "value after emplacing existing element";
    ASSERT_EQ(0, A::ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of existing element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::move_assign) << "emplace of existing element";
}

TEST(DenseHashMapExtKeyTest, OperatorBrackets)
{
    Map map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);
    map[B{"new one"}] = 4;

    A::reset();
    EXPECT_EQ(0, map[hello_str]) << "access to 'Hello' element";
    EXPECT_EQ(2, map[world_str]) << "access to 'world' element";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";

    EXPECT_EQ(0, map[std::string{"missing"}]) << "access to 'missing' element";
    ASSERT_EQ(1, A::ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::str_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "access with insertion and moving of key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "access with insertion and moving of key substitute";

    A::reset();
    EXPECT_EQ(0, map[B{hello_str}]) << "access to 'Hello' element with indirect key";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";

    EXPECT_EQ(0, map[B{"another one"}]) << "access to 'another one' element";
    ASSERT_EQ(1, A::ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::str_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "access with insertion and moving of key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "access with insertion and moving of key substitute";

    B bb("the third one");
    A::reset();
    EXPECT_EQ(0, map[bb]) << "access to 'the third one' element";
    ASSERT_EQ(1, A::ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::str_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(1, A::str_copy_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "access with insertion and moving of key substitute";
}

TEST(WrappedDenseHashSetExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, WrappedSet> AcceptsA;
    typedef container_key_accepts<B, WrappedSet> AcceptsB;
    typedef container_key_accepts<std::string, WrappedSet> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(WrappedDenseHashSetExtKeyTest, Find)
{
    WrappedSet set;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = set.emplace("Hello").first;
    auto comma_it = set.emplace(",").first;
    auto world_it = set.emplace("world").first;
    set.emplace("!");

    A::reset();
    auto hello_it_f = set.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = set.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = set.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = set.find("!");
    EXPECT_FALSE(it == set.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(WrappedDenseHashSetExtKeyTest, Count)
{
    WrappedSet set;
    const std::string hello_str("Hello"), world_str("world");
    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, set.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, set.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, set.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(WrappedDenseHashSetExtKeyTest, EqualRange)
{
    WrappedSet set;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = set.emplace("Hello").first;
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    auto range = set.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = set.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == set.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(WrappedDenseHashSetExtKeyTest, Erase)
{
    WrappedSet set;
    const std::string hello_str("Hello"), world_str("world");
    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, set.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, set.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::copy_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "erase by acceptable key substitute";
}

TEST(WrappedDenseHashSetExtKeyTest, Emplace)
{
    WrappedSet set;

    set.emplace(B{"Hello"});
    set.emplace(B{","});
    set.emplace(B{"world"});

    A::reset();
    EXPECT_FALSE(set.emplace(B{"Hello"}).second) << "emplace of existing element";
    ASSERT_EQ(0, A::ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of existing element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::move_assign) << "emplace of existing element";

    A::reset();
    set.emplace(std::string{"New one"});
    ASSERT_EQ(1, A::ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of new element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::move_assign) << "emplace of new element";
    ASSERT_EQ(0, A::str_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of new element";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of new element";

    A::reset();
    set.emplace(B{"New one Second"});
    ASSERT_EQ(1, A::ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of new element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::move_assign) << "emplace of new element";
    ASSERT_EQ(0, A::str_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of new element";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of new element";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of new element";

    A::reset();
    set.emplace("New one");
    ASSERT_EQ(1, A::ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of new constructed element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::move_assign) << "emplace of new constructed element";
    ASSERT_EQ(1, A::str_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_move_ctor) << "emplace of new constructed element";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of new constructed element";
}

TEST(WrappedDenseHashSetLookupExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, WrappedSetLookup> AcceptsA;
    typedef container_key_accepts<B, WrappedSetLookup> AcceptsB;
    typedef container_key_accepts<std::string, WrappedSetLookup> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(WrappedDenseHashSetLookupExtKeyTest, Find)
{
    WrappedSetLookup set;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = set.emplace("Hello").first;
    auto comma_it = set.emplace(",").first;
    auto world_it = set.emplace("world").first;
    set.emplace("!");

    A::reset();
    auto hello_it_f = set.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = set.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = set.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = set.find("!");
    EXPECT_FALSE(it == set.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(WrappedDenseHashSetLookupExtKeyTest, Count)
{
    WrappedSetLookup set;
    const std::string hello_str("Hello"), world_str("world");
    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, set.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, set.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, set.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(WrappedDenseHashSetLookupExtKeyTest, EqualRange)
{
    WrappedSetLookup set;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = set.emplace("Hello").first;
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    auto range = set.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = set.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == set.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(WrappedDenseHashSetLookupExtKeyTest, Erase)
{
    WrappedSetLookup set;
    const std::string hello_str("Hello"), world_str("world");
    set.emplace("Hello");
    set.emplace(",");
    set.emplace("world");
    set.emplace("!");

    A::reset();
    EXPECT_EQ(1, set.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, set.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, set.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::copy_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "erase by acceptable key substitute";
}

TEST(WrappedDenseHashMapExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, WrappedMap> AcceptsA;
    typedef container_key_accepts<B, WrappedMap> AcceptsB;
    typedef container_key_accepts<std::string, WrappedMap> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(WrappedDenseHashMapExtKeyTest, Find)
{
    WrappedMap map;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = map.emplace("Hello", 0).first;
    auto comma_it = map.emplace(",", 1).first;
    auto world_it = map.emplace("world", 2).first;
    map.emplace("!", 3);

    A::reset();
    auto hello_it_f = map.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = map.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = map.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = map.find("!");
    EXPECT_FALSE(it == map.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(WrappedDenseHashMapExtKeyTest, Count)
{
    WrappedMap map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, map.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, map.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, map.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(WrappedDenseHashMapExtKeyTest, EqualRange)
{
    WrappedMap map;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = map.emplace("Hello", 0).first;
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    auto range = map.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = map.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == map.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(WrappedDenseHashMapExtKeyTest, Erase)
{
    WrappedMap map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, map.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, map.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(2, A::ctor) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::move_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
}

TEST(WrappedDenseHashMapExtKeyTest, Emplace)
{
    WrappedMap map;

    // emplace key substitute, from which key cannot be constructed
    // without helper
    A::reset();
    auto res = map.emplace(B{"Hello"}, 0);
    ASSERT_EQ(1, A::ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::copy_assign) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::move_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::move_assign) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of indirect key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of indirect key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of indirect key substitute";

    // emplace key substitue, from which key can be constructed
    A::reset();
    map.emplace(std::string{","}, 1);
    ASSERT_EQ(1, A::ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::copy_assign) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::move_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::move_assign) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of direct key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of direct key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of direct key substitute";

    // emplace value, from which key can be constructed, but which
    // cannot be used as a key substitute
    A::reset();
    map.emplace("world", 2);
    ASSERT_EQ(1, A::ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::copy_assign) << "emplace of a value, convertible to key";
    ASSERT_EQ(1, A::move_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::move_assign) << "emplace of a value, convertible to key";
    ASSERT_EQ(1, A::str_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::str_copy_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::str_copy_assign) << "emplace of a value, convertible to key";
    ASSERT_EQ(1, A::str_move_ctor) << "emplace of a value, convertible to key";
    ASSERT_EQ(0, A::str_move_assign) << "emplace of a value, convertible to key";

    A::reset();
    EXPECT_FALSE(map.emplace(B{"Hello"}, 3).second) << "emplace of existing element";
    EXPECT_EQ(0, res.first->second) << "value after emplacing existing element";
    ASSERT_EQ(0, A::ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::copy_assign) << "emplace of existing element";
    ASSERT_EQ(0, A::move_ctor) << "emplace of existing element";
    ASSERT_EQ(0, A::move_assign) << "emplace of existing element";
}

TEST(WrappedDenseHashMapExtKeyTest, OperatorBrackets)
{
    WrappedMap map;
    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);
    map[B{"new one"}] = 4;

    A::reset();
    EXPECT_EQ(0, map[hello_str]) << "access to 'Hello' element";
    EXPECT_EQ(2, map[world_str]) << "access to 'world' element";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";

    EXPECT_EQ(0, map[std::string{"missing"}]) << "access to 'missing' element";
    ASSERT_EQ(1, A::ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::str_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "access with insertion and moving of key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "access with insertion and moving of key substitute";

    A::reset();
    EXPECT_EQ(0, map[B{hello_str}]) << "access to 'Hello' element with indirect key";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";

    EXPECT_EQ(0, map[B{"another one"}]) << "access to 'another one' element";
    ASSERT_EQ(1, A::ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::str_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "access with insertion and moving of key substitute";
    ASSERT_EQ(1, A::str_move_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "access with insertion and moving of key substitute";

    B bb("the third one");
    A::reset();
    EXPECT_EQ(0, map[bb]) << "access to 'the third one' element";
    ASSERT_EQ(1, A::ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access with insertion by acceptable key substitute";
    ASSERT_EQ(0, A::str_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(1, A::str_copy_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_copy_assign) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_ctor) << "access with insertion and moving of key substitute";
    ASSERT_EQ(0, A::str_move_assign) << "access with insertion and moving of key substitute";
}

TEST(WrappedDenseHashMapExtKeyTest, At)
{
    WrappedMap map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(0, map.at(hello_str)) << "access to 'Hello' element";
    EXPECT_EQ(1, map.at(std::string{","})) << "access to ',' element";
    EXPECT_EQ(2, map.at(B{world_str})) << "access to 'world' element";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";
}

TEST(WrappedDenseHashMapLookupExtKeyTest, TypeCheck)
{
    typedef container_key_accepts<A, WrappedMapLookup> AcceptsA;
    typedef container_key_accepts<B, WrappedMapLookup> AcceptsB;
    typedef container_key_accepts<std::string, WrappedMapLookup> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsB::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(WrappedDenseHashMapLookupExtKeyTest, Find)
{
    WrappedMapLookup map;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = map.emplace("Hello", 0).first;
    auto comma_it = map.emplace(",", 1).first;
    auto world_it = map.emplace("world", 2).first;
    map.emplace("!", 3);

    A::reset();
    auto hello_it_f = map.find(hello_str);
    EXPECT_TRUE(hello_it == hello_it_f) << "find 'Hello' string";
    auto world_it_f = map.find(world_str);
    EXPECT_TRUE(world_it == world_it_f) << "find 'world' string";
    auto comma_it_f = map.find(std::string{","});
    EXPECT_TRUE(comma_it == comma_it_f) << "find ',' string";
    ASSERT_EQ(0, A::ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "find by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "find by acceptable key substitute";

    auto it = map.find("!");
    EXPECT_FALSE(it == map.end());
    ASSERT_EQ(1, A::ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "find by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "find by implicitly constructed key";
}

TEST(WrappedDenseHashMapLookupExtKeyTest, Count)
{
    WrappedMapLookup map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.count(hello_str)) << "count 'Hello' string";
    EXPECT_EQ(1, map.count(world_str)) << "count 'world' string";
    EXPECT_EQ(0, map.count(std::string{"missing"})) << "count 'missing' string";
    ASSERT_EQ(0, A::ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "count by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "count by acceptable key substitute";

    EXPECT_EQ(1, map.count("!")) << "count '!' string";
    ASSERT_EQ(1, A::ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::copy_assign) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_ctor) << "count by implicitly constructed key";
    ASSERT_EQ(0, A::move_assign) << "count by implicitly constructed key";
}

TEST(WrappedDenseHashMapLookupExtKeyTest, EqualRange)
{
    WrappedMapLookup map;
    const std::string hello_str("Hello"), world_str("world");
    auto hello_it = map.emplace("Hello", 0).first;
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    auto range = map.equal_range(hello_str);
    EXPECT_FALSE(range.first == range.second) << "equal_range for 'Hello' string";
    EXPECT_TRUE(range.first == hello_it) << "equal_range for 'Hello' string";
    auto empty_range = map.equal_range(std::string{"missing"});
    EXPECT_TRUE(empty_range.first == empty_range.second) << "equal_range for 'missing' string";
    EXPECT_TRUE(empty_range.first == map.end()) << "equal_range for 'missing' string";
    ASSERT_EQ(0, A::ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "equal_range by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "equal_range by acceptable key substitute";
}

TEST(WrappedDenseHashMapLookupExtKeyTest, Erase)
{
    WrappedMapLookup map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(1, map.erase(hello_str)) << "erase for 'Hello' string";
    EXPECT_EQ(1, map.erase(world_str)) << "erase for 'world' string";
    EXPECT_EQ(0, map.erase(std::string{"missing"})) << "erase for 'missing' string";
    ASSERT_EQ(2, A::ctor) << "erase by acceptable key substitute"; // set deleted on 2 entries
    ASSERT_EQ(0, A::copy_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "erase by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "erase by acceptable key substitute";
    ASSERT_EQ(2, A::move_assign) << "erase by acceptable key substitute"; // set deleted on 2 entries
}

TEST(WrappedDenseHashMapLookupExtKeyTest, At)
{
    WrappedMapLookup map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(0, map.at(hello_str)) << "access to 'Hello' element";
    EXPECT_EQ(1, map.at(std::string{","})) << "access to ',' element";
    EXPECT_EQ(2, map.at(B{world_str})) << "access to 'world' element";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";
}

