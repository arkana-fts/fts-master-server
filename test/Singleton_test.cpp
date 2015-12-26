#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../Singleton.h"

using namespace FTS;

class TestSingleton : public Singleton<TestSingleton>
{
public:
   TestSingleton() {};
   virtual ~TestSingleton() {};
};

class TestSingletonFixture
{
public:
    TestSingletonFixture() {}
    ~TestSingletonFixture()
    {
        delete TestSingleton::getSingletonPtr();
    }
};

TEST_CASE_METHOD(TestSingletonFixture, "Create Test Singleton", "[Singleton]")
{
    new TestSingleton();
    REQUIRE( TestSingleton::getSingletonPtr() != nullptr );
}

TEST_CASE_METHOD( TestSingletonFixture, "Create Test Singleton twice", "[Singleton]" )
{
    new TestSingleton();
    REQUIRE( TestSingleton::getSingletonPtr() != nullptr );
    REQUIRE_THROWS( new TestSingleton() );
}

TEST_CASE_METHOD( TestSingletonFixture, "Create and destroy Test Singleton", "[Singleton]" )
{
    REQUIRE( TestSingleton::getSingletonPtr() == nullptr );
    REQUIRE_NOTHROW( new TestSingleton() );
    REQUIRE( TestSingleton::getSingletonPtr() != nullptr );
    delete TestSingleton::getSingletonPtr();
    REQUIRE_THROWS( TestSingleton::getSingleton() );
}
