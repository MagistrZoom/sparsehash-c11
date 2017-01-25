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

    static void reset()
    {
        ctor = 0;
        copy_ctor = 0;
        copy_assign = 0;
        move_ctor = 0;
        move_assign = 0;
    }

    A() { ++ctor; }
    A(const std::string& s): str_(s) { ++ctor; }
    A(std::string&& s): str_(std::move(s)) { ++ctor; }
    A(const char* s): str_(s) { ++ctor; }

    A(const A& a): str_(a.str_) { ++copy_ctor; }
    A& operator=(const A& a) { str_ = a.str_; ++copy_assign; return *this; }

    A(A&& a): str_(std::move(a.str_)) { ++move_ctor; }
    A& operator=(A&& a) { str_ = std::move(a.str_); ++move_assign; return *this; }

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

struct HashA
    : public std::hash<std::string>
{
    size_t operator() (const A& a) const { return std::hash<std::string>::operator() (a.get_str()); }
    size_t operator() (const std::string& s) const { return std::hash<std::string>::operator() (s); }
};

struct EqualA
{
    bool operator() (const A& lhs, const A& rhs) const { return lhs == rhs; }
    bool operator() (const A& lhs, const std::string& rhs) const { return lhs == rhs; }
    bool operator() (const std::string& lhs, const A& rhs) const { return lhs == rhs; }
};

template <class K, class Container>
struct container_key_accepts
    : public std::integral_constant<bool, Container::template accept_as_key<K>::value>
{};

}

using namespace extkey_tests;

TEST(DenseHashSetExtKeyTest, TypeCheck)
{
    typedef dense_hash_set<A, HashA, EqualA> Set;
    typedef container_key_accepts<A, Set> AcceptsA;
    typedef container_key_accepts<std::string, Set> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(DenseHashSetExtKeyTest, Find)
{
    dense_hash_set<A, HashA, EqualA> set;
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
    dense_hash_set<A, HashA, EqualA> set;
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
    dense_hash_set<A, HashA, EqualA> set;
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
    dense_hash_set<A, HashA, EqualA> set;
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

TEST(DenseHashMapExtKeyTest, TypeCheck)
{
    typedef dense_hash_map<A, int, HashA, EqualA> Map;
    typedef container_key_accepts<A, Map> AcceptsA;
    typedef container_key_accepts<std::string, Map> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(DenseHashMapExtKeyTest, Find)
{
    dense_hash_map<A, int, HashA, EqualA> map;
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
    dense_hash_map<A, int, HashA, EqualA> map;
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
    dense_hash_map<A, int, HashA, EqualA> map;
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
    dense_hash_map<A, int, HashA, EqualA> map;
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

TEST(DenseHashMapExtKeyTest, OperatorBrackets)
{
    dense_hash_map<A, int, HashA, EqualA> map;
    map.set_empty_key(A{"<empty>"});
    map.set_deleted_key(A{"<deleted>"});

    const std::string hello_str("Hello"), world_str("world");

    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

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
}

TEST(WrappedDenseHashSetExtKeyTest, TypeCheck)
{
    typedef wrapped_dense_hash_set<A, HashA, EqualA> Set;
    typedef container_key_accepts<A, Set> AcceptsA;
    typedef container_key_accepts<std::string, Set> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(WrappedDenseHashSetExtKeyTest, Find)
{
    wrapped_dense_hash_set<A, HashA, EqualA> set;
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
    wrapped_dense_hash_set<A, HashA, EqualA> set;
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
    wrapped_dense_hash_set<A, HashA, EqualA> set;
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
    wrapped_dense_hash_set<A, HashA, EqualA> set;
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
    typedef wrapped_dense_hash_map<A, int, HashA, EqualA> Map;
    typedef container_key_accepts<A, Map> AcceptsA;
    typedef container_key_accepts<std::string, Map> AcceptsString;
    EXPECT_TRUE(AcceptsA::value);
    EXPECT_TRUE(AcceptsString::value);
}

TEST(WrappedDenseHashMapExtKeyTest, Find)
{
    wrapped_dense_hash_map<A, int, HashA, EqualA> map;
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
    wrapped_dense_hash_map<A, int, HashA, EqualA> map;
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
    wrapped_dense_hash_map<A, int, HashA, EqualA> map;
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
    wrapped_dense_hash_map<A, int, HashA, EqualA> map;
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

TEST(WrappedDenseHashMapExtKeyTest, OperatorBrackets)
{
    wrapped_dense_hash_map<A, int, HashA, EqualA> map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

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
}

TEST(WrappedDenseHashMapExtKeyTest, At)
{
    wrapped_dense_hash_map<A, int, HashA, EqualA> map;
    const std::string hello_str("Hello"), world_str("world");
    map.emplace("Hello", 0);
    map.emplace(",", 1);
    map.emplace("world", 2);
    map.emplace("!", 3);

    A::reset();
    EXPECT_EQ(0, map.at(hello_str)) << "access to 'Hello' element";
    EXPECT_EQ(1, map.at(std::string{","})) << "access to ',' element";
    EXPECT_EQ(2, map.at(world_str)) << "access to 'world' element";
    ASSERT_EQ(0, A::ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::copy_assign) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_ctor) << "access by acceptable key substitute";
    ASSERT_EQ(0, A::move_assign) << "access by acceptable key substitute";
}

