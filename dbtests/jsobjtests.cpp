// jsobjtests.cpp - Tests for jsobj.{h,cpp} code
//

/**
 *    Copyright (C) 2008 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../db/jsobj.h"
#include "../db/jsobjmanipulator.h"
#include "../db/json.h"
#include "../db/repl.h"

#include "dbtests.h"

namespace JsobjTests {
    class BSONElementBasic {
    public:
        void run() {
            ASSERT_EQUALS( 1, BSONElement().size() );
        }
    };
    
    namespace BSONObjTests {
        class Create {
        public:
            void run() {
                BSONObj b;
                ASSERT_EQUALS( 0, b.nFields() );
            }
        };

        class Base {
        protected:
            static BSONObj basic( const char *name, int val ) {
                BSONObjBuilder b;
                b.append( name, val );
                return b.obj();
            }
            static BSONObj basic( const char *name, vector< int > val ) {
                BSONObjBuilder b;
                b.append( name, val );
                return b.obj();
            }
            template< class T >
            static BSONObj basic( const char *name, T val ) {
                BSONObjBuilder b;
                b.append( name, val );
                return b.obj();
            }
        };

        class WoCompareBasic : public Base {
        public:
            void run() {
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 1 ) ) == 0 );
                ASSERT( basic( "a", 2 ).woCompare( basic( "a", 1 ) ) > 0 );
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 2 ) ) < 0 );
                // field name comparison
                ASSERT( basic( "a", 1 ).woCompare( basic( "b", 1 ) ) < 0 );
            }
        };

        class NumericCompareBasic : public Base {
        public:
            void run() {
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 1.0 ) ) == 0 );
            }
        };

        class WoCompareEmbeddedObject : public Base {
        public:
            void run() {
                ASSERT( basic( "a", basic( "b", 1 ) ).woCompare
                        ( basic( "a", basic( "b", 1.0 ) ) ) == 0 );
                ASSERT( basic( "a", basic( "b", 1 ) ).woCompare
                        ( basic( "a", basic( "b", 2 ) ) ) < 0 );
            }
        };

        class WoCompareEmbeddedArray : public Base {
        public:
            void run() {
                vector< int > i;
                i.push_back( 1 );
                i.push_back( 2 );
                vector< double > d;
                d.push_back( 1 );
                d.push_back( 2 );
                ASSERT( basic( "a", i ).woCompare( basic( "a", d ) ) == 0 );

                vector< int > j;
                j.push_back( 1 );
                j.push_back( 3 );
                ASSERT( basic( "a", i ).woCompare( basic( "a", j ) ) < 0 );
            }
        };

        class WoCompareOrdered : public Base {
        public:
            void run() {
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 1 ), basic( "a", 1 ) ) == 0 );
                ASSERT( basic( "a", 2 ).woCompare( basic( "a", 1 ), basic( "a", 1 ) ) > 0 );
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 2 ), basic( "a", 1 ) ) < 0 );
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 1 ), basic( "a", -1 ) ) == 0 );
                ASSERT( basic( "a", 2 ).woCompare( basic( "a", 1 ), basic( "a", -1 ) ) < 0 );
                ASSERT( basic( "a", 1 ).woCompare( basic( "a", 2 ), basic( "a", -1 ) ) > 0 );
            }
        };
        
        class WoCompareDifferentLength : public Base {
        public:
            void run() {
                ASSERT( BSON( "a" << 1 ).woCompare( BSON( "a" << 1 << "b" << 1 ) ) < 0 );
                ASSERT( BSON( "a" << 1 << "b" << 1 ).woCompare( BSON( "a" << 1 ) ) > 0 );
            }
        };
        
        class WoSortOrder : public Base {
        public:
            void run() {
                ASSERT( BSON( "a" << 1 ).woSortOrder( BSON( "a" << 2 ), BSON( "b" << 1 << "a" << 1 ) ) < 0 );
                ASSERT( fromjson( "{a:null}" ).woSortOrder( BSON( "b" << 1 ), BSON( "a" << 1 ) ) == 0 );
            }
        };
        
        class TimestampTest : public Base {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendTimestamp( "a" );
                BSONObj o = b.done();
                o.toString();
                ASSERT( o.valid() );
                ASSERT_EQUALS( Timestamp, o.getField( "a" ).type() );
                BSONObjIterator i( o );
                ASSERT( i.more() );
                BSONElement e = i.next();
                ASSERT_EQUALS( Timestamp, e.type() );
                ASSERT( i.more() );
                e = i.next();
                ASSERT( e.eoo() );
                
                OpTime before = OpTime::now();
                BSONElementManipulator( o.firstElement() ).initTimestamp();
                OpTime after = OpTime::now();
                
                OpTime test = OpTime( o.firstElement().date() );
                ASSERT( before < test && test < after );
                
                BSONElementManipulator( o.firstElement() ).initTimestamp();
                test = OpTime( o.firstElement().date() );
                ASSERT( before < test && test < after );  
            }
        };
        
        namespace Validation {
            
            class Base {
            public:
                virtual ~Base() {}
                void run() {
                    ASSERT( valid().valid() );
                    ASSERT( !invalid().valid() );
                }
            protected:
                virtual BSONObj valid() const { return BSONObj(); }
                virtual BSONObj invalid() const { return BSONObj(); }
                static char get( const BSONObj &o, int i ) {
                    return o.objdata()[ i ];
                }
                static void set( BSONObj &o, int i, char c ) {
                    const_cast< char * >( o.objdata() )[ i ] = c;
                }
            };
            
            class BadType : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":1}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 4, 50 );
                    return ret;
                }
            };

            class EooBeforeEnd : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":1}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    // (first byte of size)++
                    set( ret, 0, get( ret, 0 ) + 1 );
                    // re-read size for BSONObj::details
                    return ret.copy();
                }
            };

            class Undefined : public Base {
            public:
                void run() {
                    BSONObjBuilder b;
                    b.appendNull( "a" );
                    BSONObj o = b.done();
                    set( o, 4, mongo::Undefined );
                    ASSERT( o.valid() );
                }
            };
            
            class TotalSizeTooSmall : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":1}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    // (first byte of size)--
                    set( ret, 0, get( ret, 0 ) - 1 );
                    // re-read size for BSONObj::details
                    return ret.copy();                    
                }
            };

            class EooMissing : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":1}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, ret.objsize() - 1, 0xff );
                    // (first byte of size)--
                    set( ret, 0, get( ret, 0 ) - 1 );
                    // re-read size for BSONObj::details
                    return ret.copy();                    
                }
            };
            
            class WrongStringSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":\"b\"}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 0, get( ret, 0 ) + 1 );
                    set( ret, 7, get( ret, 7 ) + 1 );
                    return ret.copy();       
                }
            };

            class ZeroStringSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":\"b\"}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 7, 0 );
                    return ret;
                }
            };

            class NegativeStringSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":\"b\"}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 10, -100 );
                    return ret;
                }
            };

            class WrongSubobjectSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":{\"b\":1}}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 0, get( ret, 0 ) + 1 );
                    set( ret, 7, get( ret, 7 ) + 1 );
                    return ret.copy();       
                }
            };            

            class WrongDbrefNsSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{ \"a\": Dbref( \"b\", \"ffffffffffffffffffffffff\" ) }" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 0, get( ret, 0 ) + 1 );
                    set( ret, 7, get( ret, 7 ) + 1 );
                    return ret.copy();
                };
            };
            
            class WrongSymbolSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":\"b\"}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 4, Symbol );
                    set( ret, 0, get( ret, 0 ) + 1 );
                    set( ret, 7, get( ret, 7 ) + 1 );
                    return ret.copy();       
                }
            };

            class WrongCodeSize : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":\"b\"}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    set( ret, 4, Code );
                    set( ret, 0, get( ret, 0 ) + 1 );
                    set( ret, 7, get( ret, 7 ) + 1 );
                    return ret.copy();       
                }
            };

            class NoFieldNameEnd : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":1}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    memset( const_cast< char * >( ret.objdata() ) + 5, 0xff, ret.objsize() - 5 );
                    return ret;
                }                
            };
            
            class BadRegex : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":/c/i}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    memset( const_cast< char * >( ret.objdata() ) + 7, 0xff, ret.objsize() - 7 );
                    return ret;
                }                
            };
            
            class BadRegexOptions : public Base {
                BSONObj valid() const {
                    return fromjson( "{\"a\":/c/i}" );
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    memset( const_cast< char * >( ret.objdata() ) + 9, 0xff, ret.objsize() - 9 );
                    return ret;
                }                
            };

            class CodeWScopeBase : public Base {
                BSONObj valid() const {
                    BSONObjBuilder b;
                    BSONObjBuilder scope;
                    scope.append( "a", "b" );
                    b.appendCodeWScope( "c", "d", scope.done() );
                    return b.obj();
                }
                BSONObj invalid() const {
                    BSONObj ret = valid();
                    modify( ret );
                    return ret;
                }
            protected:
                virtual void modify( BSONObj &o ) const = 0;
            };
            
            class CodeWScopeSmallSize : public CodeWScopeBase {
                void modify( BSONObj &o ) const {
                    set( o, 7, 7 );
                }
            };
            
            class CodeWScopeZeroStrSize : public CodeWScopeBase {
                void modify( BSONObj &o ) const {
                    set( o, 11, 0 );
                }
            };

            class CodeWScopeSmallStrSize : public CodeWScopeBase {
                void modify( BSONObj &o ) const {
                    set( o, 11, 1 );
                }
            };
            
            class CodeWScopeNoSizeForObj : public CodeWScopeBase {
                void modify( BSONObj &o ) const {
                    set( o, 7, 13 );
                }
            };            
            
            class CodeWScopeSmallObjSize : public CodeWScopeBase {
                void modify( BSONObj &o ) const {
                    set( o, 17, 1 );
                }
            };            

            class CodeWScopeBadObject : public CodeWScopeBase {
                void modify( BSONObj &o ) const {
                    set( o, 21, JSTypeMax + 1 );
                }
            };            

            class NoSize {
            public:
                NoSize( BSONType type ) : type_( type ) {}
                void run() {
                    const char data[] = { 0x07, 0x00, 0x00, 0x00, char( type_ ), 'a', 0x00 };
                    BSONObj o( data );
                    ASSERT( !o.valid() );
                }
            private:
                BSONType type_;
            };
            
            // Randomized BSON parsing test.  See if we seg fault.
            class Fuzz {
            public:
                Fuzz( double frequency ) : frequency_( frequency ) {}
                void run() {
                    BSONObj a = fromjson( "{\"a\": 1, \"b\": \"c\"}" );
                    fuzz( a );
                    a.valid();
                    
                    BSONObj b = fromjson( "{\"one\":2, \"two\":5, \"three\": {},"
                                         "\"four\": { \"five\": { \"six\" : 11 } },"
                                         "\"seven\": [ \"a\", \"bb\", \"ccc\", 5 ],"
                                         "\"eight\": Dbref( \"rrr\", \"01234567890123456789aaaa\" ),"
                                         "\"_id\": ObjectId( \"deadbeefdeadbeefdeadbeef\" ),"
                                         "\"nine\": { \"$binary\": \"abc=\", \"$type\": \"02\" },"
                                         "\"ten\": Date( 44 ), \"eleven\": /foooooo/i }" );
                    fuzz( b );
                    b.valid();
                }
            private:
                void fuzz( BSONObj &o ) const {
                    for( int i = 4; i < o.objsize(); ++i )
                        for( unsigned char j = 1; j; j <<= 1 )
                            if ( rand() < int( RAND_MAX * frequency_ ) ) {
                                char *c = const_cast< char * >( o.objdata() ) + i;
                                if ( *c & j )
                                    *c &= ~j;
                                else
                                    *c |= j;
                            }                    
                }
                double frequency_;
            };
            
        } // namespace Validation        
        
    } // namespace BSONObjTests
    
    namespace OIDTests {
        
        class init1 {
        public:
            void run(){
                OID a;
                OID b;
                
                a.init();
                b.init();
                
                ASSERT( a != b );
            }
        };

        class initParse1 {
        public:
            void run(){

                OID a;
                OID b;
                
                a.init();
                b.init( a.str() );
                
                ASSERT( a == b );
            }
        };
    } // namespace OIDTests
    
    namespace ValueStreamTests {
        
        class LabelBase {
        public:
            virtual ~LabelBase() {}
            void run() {
                ASSERT( !expected().woCompare( actual() ) );
            }
        protected:
            virtual BSONObj expected() = 0;
            virtual BSONObj actual() = 0;
        };
        
        class LabelBasic : public LabelBase {
            BSONObj expected() {
                return BSON( "a" << ( BSON( "$gt" << 1 ) ) );
            }
            BSONObj actual() {
                return BSON( "a" << GT << 1 );
            }            
        };

        class LabelShares : public LabelBase {
            BSONObj expected() {
                return BSON( "z" << "q" << "a" << ( BSON( "$gt" << 1 ) ) << "x" << "p" );
            }
            BSONObj actual() {
                return BSON( "z" << "q" << "a" << GT << 1 << "x" << "p" );
            }            
        };        
        
        class LabelDouble : public LabelBase {
            BSONObj expected() {
                return BSON( "a" << ( BSON( "$gt" << 1 << "$lte" << "x" ) ) );
            }
            BSONObj actual() {
                return BSON( "a" << GT << 1 << LTE << "x" );
            }            
        };        

        class LabelDoubleShares : public LabelBase {
            BSONObj expected() {
                return BSON( "z" << "q" << "a" << ( BSON( "$gt" << 1 << "$lte" << "x" ) ) << "x" << "p" );
            }
            BSONObj actual() {
                return BSON( "z" << "q" << "a" << GT << 1 << LTE << "x" << "x" << "p" );
            }            
        };        

        class LabelMulti : public LabelBase {
            BSONObj expected() {
                return BSON( "z" << "q"
                            << "a" << BSON( "$gt" << 1 << "$lte" << "x" )
                            << "b" << BSON( "$ne" << 1 << "$ne" << "f" << "$ne" << 22.3 )
                            << "x" << "p" );
            }
            BSONObj actual() {
                return BSON( "z" << "q"
                            << "a" << GT << 1 << LTE << "x"
                            << "b" << NE << 1 << NE << "f" << NE << 22.3
                            << "x" << "p" );
            }            
        };        
        
        class Unallowed {
        public:
            void run() {
                ASSERT_EXCEPTION( BSON( GT << 4 ), MsgAssertionException );
                ASSERT_EXCEPTION( BSON( "a" << 1 << GT << 4 ), MsgAssertionException );
            }
        };
        
    } // namespace ValueStreamTests
    
    class All : public UnitTest::Suite {
    public:
        All() {
            add< BSONElementBasic >();
            add< BSONObjTests::Create >();
            add< BSONObjTests::WoCompareBasic >();
            add< BSONObjTests::NumericCompareBasic >();
            add< BSONObjTests::WoCompareEmbeddedObject >();
            add< BSONObjTests::WoCompareEmbeddedArray >();
            add< BSONObjTests::WoCompareOrdered >();
            add< BSONObjTests::WoCompareDifferentLength >();
            add< BSONObjTests::WoSortOrder >();
            add< BSONObjTests::TimestampTest >();
            add< BSONObjTests::Validation::BadType >();
            add< BSONObjTests::Validation::EooBeforeEnd >();
            add< BSONObjTests::Validation::Undefined >();
            add< BSONObjTests::Validation::TotalSizeTooSmall >();
            add< BSONObjTests::Validation::EooMissing >();
            add< BSONObjTests::Validation::WrongStringSize >();
            add< BSONObjTests::Validation::ZeroStringSize >();
            add< BSONObjTests::Validation::NegativeStringSize >();
            add< BSONObjTests::Validation::WrongSubobjectSize >();
            add< BSONObjTests::Validation::WrongDbrefNsSize >();
            add< BSONObjTests::Validation::WrongSymbolSize >();
            add< BSONObjTests::Validation::WrongCodeSize >();
            add< BSONObjTests::Validation::NoFieldNameEnd >();
            add< BSONObjTests::Validation::BadRegex >();
            add< BSONObjTests::Validation::BadRegexOptions >();
            add< BSONObjTests::Validation::CodeWScopeSmallSize >();
            add< BSONObjTests::Validation::CodeWScopeZeroStrSize >();
            add< BSONObjTests::Validation::CodeWScopeSmallStrSize >();
            add< BSONObjTests::Validation::CodeWScopeNoSizeForObj >();
            add< BSONObjTests::Validation::CodeWScopeSmallObjSize >();
            add< BSONObjTests::Validation::CodeWScopeBadObject >();
            add< BSONObjTests::Validation::NoSize >( Symbol );
            add< BSONObjTests::Validation::NoSize >( Code );
            add< BSONObjTests::Validation::NoSize >( String );
            add< BSONObjTests::Validation::NoSize >( CodeWScope );
            add< BSONObjTests::Validation::NoSize >( DBRef );
            add< BSONObjTests::Validation::NoSize >( Object );
            add< BSONObjTests::Validation::NoSize >( Array );
            add< BSONObjTests::Validation::NoSize >( BinData );
            add< BSONObjTests::Validation::Fuzz >( .5 );
            add< BSONObjTests::Validation::Fuzz >( .1 );
            add< BSONObjTests::Validation::Fuzz >( .05 );
            add< BSONObjTests::Validation::Fuzz >( .01 );
            add< BSONObjTests::Validation::Fuzz >( .001 );
            add< OIDTests::init1 >();
            add< OIDTests::initParse1 >();
            add< ValueStreamTests::LabelBasic >();
            add< ValueStreamTests::LabelShares >();
            add< ValueStreamTests::LabelDouble >();
            add< ValueStreamTests::LabelDoubleShares >();
            add< ValueStreamTests::LabelMulti >();
            add< ValueStreamTests::Unallowed >();
        }
    };
    
} // namespace JsobjTests

UnitTest::TestPtr jsobjTests() {
    return UnitTest::createSuite< JsobjTests::All >();
}
