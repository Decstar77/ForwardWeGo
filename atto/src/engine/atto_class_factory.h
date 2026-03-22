#pragma once

#include "atto_core.h"

namespace atto {
    template < typename Base >
    class ClassFactory {
    public:
        template < typename Derived >
        static void Register( const char * name, i64 key ) {
            Entry entry;
            entry.key = key;
            entry.name = name;
            entry.create = []() -> Base * { return new Derived(); };
            GetEntries().push_back( entry );
        }

        static Base *Create( const char * name ) {
            for ( const auto &e: GetEntries() ) {
                if ( strcmp( e.name, name ) == 0 ) {
                    return e.create();
                }
            }
            return nullptr;
        }


        static Base *Create( const i64 key ) {
            for ( const auto &e: GetEntries() ) {
                if ( e.key == key ) {
                    return e.create();
                }
            }
            return nullptr;
        }


        static std::unique_ptr< Base > CreateUnique( const char * name ) {
            return std::unique_ptr< Base >( Create( name ) );
        }

        static std::unique_ptr< Base > CreateUnique( const i64 key ) {
            return std::unique_ptr< Base >( Create( key ) );
        }

        static const std::vector< const char * > GetRegisteredNames() {
            std::vector< const char * > names;
            for ( const auto &e: GetEntries() ) {
                names.push_back( e.name );
            }
            return names;
        }

    private:
        struct Entry {
            i64 key;
            const char * name;

            Base * ( *create )();
        };

        static std::vector< Entry > &GetEntries() {
            static std::vector< Entry > entries;
            return entries;
        }
    };

    template < typename Base, typename Derived >
    struct ClassRegistrar {
        ClassRegistrar( const char * name, i64 key ) {
            ClassFactory< Base >::template Register< Derived >( name, key );
        }
    };
}

#define ATTO_REGISTER_CLASS( Base, Derived, Key ) \
    static atto::ClassRegistrar<Base, Derived> s_registrar_##Derived( Stringify( Derived ), static_cast<i64>( Key ) );
