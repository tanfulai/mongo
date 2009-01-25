// jsontests.cpp - Tests for json.{h,cpp} code and BSONObj::jsonString()
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
#include "../db/json.h"

#include "dbtests.h"

#include <limits>

namespace JsonTests {
    namespace JsonStringTests {

        class Empty {
        public:
            void run() {
                BSONObjBuilder b;
                ASSERT_EQUALS( "{}", b.done().jsonString( Strict ) );
            }
        };
        
        class SingleStringMember {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", "b" );
                ASSERT_EQUALS( "{ \"a\" : \"b\" }", b.done().jsonString( Strict ) );
            }
        };
        
        class EscapedCharacters {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", "\" \\ / \b \f \n \r \t" );
                ASSERT_EQUALS( "{ \"a\" : \"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\" }", b.done().jsonString( Strict ) );
            }
        };
        
        // per http://www.ietf.org/rfc/rfc4627.txt, control characters are
        // (U+0000 through U+001F).  U+007F is not mentioned as a control character.
        class AdditionalControlCharacters {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", "\x1 \x1f" );
                ASSERT_EQUALS( "{ \"a\" : \"\\u0001 \\u001f\" }", b.done().jsonString( Strict ) );
            }
        };
        
        class ExtendedAscii {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", "\x80" );
                ASSERT_EQUALS( "{ \"a\" : \"\x80\" }", b.done().jsonString( Strict ) );
            }
        };
        
        class EscapeFieldName {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "\t", "b" );
                ASSERT_EQUALS( "{ \"\\t\" : \"b\" }", b.done().jsonString( Strict ) );
            }
        };
        
        class SingleIntMember {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendInt( "a", 1 );
                ASSERT_EQUALS( "{ \"a\" : 1 }", b.done().jsonString( Strict ) );
            }
        };
        
        class SingleNumberMember {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", 1.5 );
                ASSERT_EQUALS( "{ \"a\" : 1.5 }", b.done().jsonString( Strict ) );
            }
        };
        
        class InvalidNumbers {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", numeric_limits< double >::infinity() );
                ASSERT_EXCEPTION( b.done().jsonString( Strict ), AssertionException );
                
                BSONObjBuilder c;
                c.append( "a", numeric_limits< double >::quiet_NaN() );
                ASSERT_EXCEPTION( c.done().jsonString( Strict ), AssertionException );
                
                BSONObjBuilder d;
                d.append( "a", numeric_limits< double >::signaling_NaN() );
                ASSERT_EXCEPTION( d.done().jsonString( Strict ), AssertionException );
            }
        };
        
        class NumberPrecision {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", 123456789 );
                ASSERT_EQUALS( "{ \"a\" : 123456789 }", b.done().jsonString( Strict ) );
            }
        };
        
        class NegativeNumber {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", -1 );
                ASSERT_EQUALS( "{ \"a\" : -1 }", b.done().jsonString( Strict ) );
            }
        };
        
        class SingleBoolMember {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendBool( "a", true );
                ASSERT_EQUALS( "{ \"a\" : true }", b.done().jsonString( Strict ) );
                
                BSONObjBuilder c;
                c.appendBool( "a", false );
                ASSERT_EQUALS( "{ \"a\" : false }", c.done().jsonString( Strict ) );
            }
        };
        
        class SingleNullMember {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendNull( "a" );
                ASSERT_EQUALS( "{ \"a\" : null }", b.done().jsonString( Strict ) );
            }
        };
        
        class SingleObjectMember {
        public:
            void run() {
                BSONObjBuilder b, c;
                b.append( "a", c.done() );
                ASSERT_EQUALS( "{ \"a\" : {} }", b.done().jsonString( Strict ) );
            }
        };
        
        class TwoMembers {
        public:
            void run() {
                BSONObjBuilder b;
                b.append( "a", 1 );
                b.append( "b", 2 );
                ASSERT_EQUALS( "{ \"a\" : 1, \"b\" : 2 }", b.done().jsonString( Strict ) );
            }
        };
        
        class EmptyArray {
        public:
            void run() {
                vector< int > arr;
                BSONObjBuilder b;
                b.append( "a", arr );
                ASSERT_EQUALS( "{ \"a\" : [] }", b.done().jsonString( Strict ) );
            }
        };
        
        class Array {
        public:
            void run() {
                vector< int > arr;
                arr.push_back( 1 );
                arr.push_back( 2 );
                BSONObjBuilder b;
                b.append( "a", arr );
                ASSERT_EQUALS( "{ \"a\" : [ 1, 2 ] }", b.done().jsonString( Strict ) );
            }
        };
        
        class DBRef {
        public:
            void run() {
                OID oid;
                memset( &oid, 0xff, 12 );
                BSONObjBuilder b;
                b.appendDBRef( "a", "namespace", oid );
                ASSERT_EQUALS( "{ \"a\" : { \"$ns\" : \"namespace\", \"$id\" : \"ffffffffffffffffffffffff\" } }",
                              b.done().jsonString( Strict ) );
                ASSERT_EQUALS( "{ \"a\" : { \"$ns\" : \"namespace\", \"$id\" : \"ffffffffffffffffffffffff\" } }",
                              b.done().jsonString( JS ) );
                ASSERT_EQUALS( "{ \"a\" : Dbref( \"namespace\", \"ffffffffffffffffffffffff\" ) }",
                              b.done().jsonString( TenGen ) );
            }
        };
        
        class DBRefZero {
        public:
            void run() {
                OID oid;
                memset( &oid, 0, 12 );
                BSONObjBuilder b;
                b.appendDBRef( "a", "namespace", oid );
                ASSERT_EQUALS( "{ \"a\" : { \"$ns\" : \"namespace\", \"$id\" : \"000000000000000000000000\" } }",
                              b.done().jsonString( Strict ) );
            }
        };
        
        class ObjectId {
        public:
            void run() {
                OID oid;
                memset( &oid, 0xff, 12 );
                BSONObjBuilder b;
                b.appendOID( "a", &oid );
                ASSERT_EQUALS( "{ \"a\" : \"ffffffffffffffffffffffff\" }",
                              b.done().jsonString( Strict ) );
                ASSERT_EQUALS( "{ \"a\" : ObjectId( \"ffffffffffffffffffffffff\" ) }",
                              b.done().jsonString( TenGen ) );
            }
        };
        
        class BinData {
        public:
            void run() {
                char z[ 3 ];
                z[ 0 ] = 'a';
                z[ 1 ] = 'b';
                z[ 2 ] = 'c';
                BSONObjBuilder b;
                b.appendBinData( "a", 3, ByteArray, z );
                ASSERT_EQUALS( "{ \"a\" : { \"$binary\" : \"YWJj\", \"$type\" : \"02\" } }",
                              b.done().jsonString( Strict ) );
                
                BSONObjBuilder c;
                c.appendBinData( "a", 2, ByteArray, z );
                ASSERT_EQUALS( "{ \"a\" : { \"$binary\" : \"YWI=\", \"$type\" : \"02\" } }",
                              c.done().jsonString( Strict ) );
                
                BSONObjBuilder d;
                d.appendBinData( "a", 1, ByteArray, z );
                ASSERT_EQUALS( "{ \"a\" : { \"$binary\" : \"YQ==\", \"$type\" : \"02\" } }",
                              d.done().jsonString( Strict ) );
            }
        };
        
        class Symbol {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendSymbol( "a", "b" );
                ASSERT_EQUALS( "{ \"a\" : \"b\" }", b.done().jsonString( Strict ) );
            }
        };
        
        class Date {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendDate( "a", 0 );
                ASSERT_EQUALS( "{ \"a\" : { \"$date\" : 0 } }", b.done().jsonString( Strict ) );
                ASSERT_EQUALS( "{ \"a\" : Date( 0 ) }", b.done().jsonString( TenGen ) );
                ASSERT_EQUALS( "{ \"a\" : Date( 0 ) }", b.done().jsonString( JS ) );
            }
        };
        
        class Regex {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendRegex( "a", "abc", "i" );
                ASSERT_EQUALS( "{ \"a\" : { \"$regex\" : \"abc\", \"$options\" : \"i\" } }",
                              b.done().jsonString( Strict ) );
                ASSERT_EQUALS( "{ \"a\" : /abc/i }", b.done().jsonString( TenGen ) );
                ASSERT_EQUALS( "{ \"a\" : /abc/i }", b.done().jsonString( JS ) );
            }
        };
        
        class RegexEscape {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendRegex( "a", "/\"", "i" );
                ASSERT_EQUALS( "{ \"a\" : { \"$regex\" : \"\\/\\\"\", \"$options\" : \"i\" } }",
                              b.done().jsonString( Strict ) );
                ASSERT_EQUALS( "{ \"a\" : /\\/\\\"/i }", b.done().jsonString( TenGen ) );
                ASSERT_EQUALS( "{ \"a\" : /\\/\\\"/i }", b.done().jsonString( JS ) );
            }
        };
        
        class RegexManyOptions {
        public:
            void run() {
                BSONObjBuilder b;
                b.appendRegex( "a", "z", "abcgimx" );
                ASSERT_EQUALS( "{ \"a\" : { \"$regex\" : \"z\", \"$options\" : \"abcgimx\" } }",
                              b.done().jsonString( Strict ) );
                ASSERT_EQUALS( "{ \"a\" : /z/gim }", b.done().jsonString( TenGen ) );
                ASSERT_EQUALS( "{ \"a\" : /z/gim }", b.done().jsonString( JS ) );
            }
        };
        
    } // namespace JsonStringTests
    
    namespace FromJsonTests {
        
        class Base {
        public:
            void run() {
                ASSERT( fromjson( json() ).valid() );
                assertEquals( bson(), fromjson( json() ) );
                assertEquals( bson(), fromjson( bson().jsonString( Strict ) ) );
                assertEquals( bson(), fromjson( bson().jsonString( TenGen ) ) );
                assertEquals( bson(), fromjson( bson().jsonString( JS ) ) );
            }
        protected:
            virtual BSONObj bson() const = 0;
            virtual string json() const = 0;
        private:
            static void assertEquals( const BSONObj &expected, const BSONObj &actual ) {
                if ( expected.woCompare( actual ) ) {
                    out() << "Expected: " << expected.toString()
                    << ", got: " << actual.toString();
                }
                ASSERT( !expected.woCompare( actual ) );
            }
        };
        
        class Bad {
        public:
            void run() {
                ASSERT_EXCEPTION( fromjson( json() ), MsgAssertionException );
            }
        protected:
            virtual string json() const = 0;
        };
        
        class Empty : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{}";
            }
        };
        
        class EmptyWithSpace : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ }";
            }
        };
        
        class SingleString : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", "b" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : \"b\" }";
            }
        };
        
        class EmptyStrings : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "", "" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"\" : \"\" }";
            }
        };
        
        class ReservedFieldName : public Bad {
            virtual string json() const {
                return "{ \"$ns\" : \"b\" }";
            }
        };
        
        class OkDollarFieldName : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "$where", 1 );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"$where\" : 1 }";
            }
        };
        
        class SingleNumber : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", 1 );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : 1 }";
            }
        };
        
        class FancyNumber {
        public:
            void run() {
                ASSERT_EQUALS( bson().firstElement().number(),
                              fromjson( json() ).firstElement().number() );
            }
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", -4.4433e-2 );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : -4.4433e-2 }";
            }
        };
        
        class TwoElements : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", 1 );
                b.append( "b", "foo" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : 1, \"b\" : \"foo\" }";
            }
        };
        
        class Subobject : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", 1 );
                BSONObjBuilder c;
                c.append( "z", b.done() );
                return c.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"z\" : { \"a\" : 1 } }";
            }
        };
        
        class ArrayEmpty : public Base {
            virtual BSONObj bson() const {
                vector< int > arr;
                BSONObjBuilder b;
                b.append( "a", arr );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : [] }";
            }
        };
        
        class Array : public Base {
            virtual BSONObj bson() const {
                vector< int > arr;
                arr.push_back( 1 );
                arr.push_back( 2 );
                arr.push_back( 3 );
                BSONObjBuilder b;
                b.append( "a", arr );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : [ 1, 2, 3 ] }";
            }
        };
        
        class True : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendBool( "a", true );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : true }";
            }
        };
        
        class False : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendBool( "a", false );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : false }";
            }
        };
        
        class Null : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendNull( "a" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : null }";
            }
        };
        
        class EscapedCharacters : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", "\" \\ / \b \f \n \r \t" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : \"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\" }";
            }
        };
        
        class AllowedControlCharacter : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "a", "\x7f" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : \"\x7f\" }";
            }
        };
        
        class EscapeFieldName : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.append( "\n", "b" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"\\n\" : \"b\" }";
            }
        };
        
        class EscapedUnicodeToUtf8 : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                char u[ 7 ];
                u[ 0 ] = 0xe0 | 0x0a;
                u[ 1 ] = 0x80;
                u[ 2 ] = 0x80;
                u[ 3 ] = 0xe0 | 0x0a;
                u[ 4 ] = 0x80;
                u[ 5 ] = 0x80;
                u[ 6 ] = 0;
                b.append( "a", u );
                ASSERT_EQUALS( string( u ), b.done().firstElement().valuestr() );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : \"\\ua000\\uA000\" }";
            }
        };
        
        class Utf8AllOnes : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                char u[ 8 ];
                u[ 0 ] = 0x01;
                
                u[ 1 ] = 0x7f;
                
                u[ 2 ] = 0xdf;
                u[ 3 ] = 0xbf;
                
                u[ 4 ] = 0xef;
                u[ 5 ] = 0xbf;
                u[ 6 ] = 0xbf;
                
                u[ 7 ] = 0;
                
                b.append( "a", u );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : \"\\u0001\\u007f\\u07ff\\uffff\" }";
            }
        };
        
        class Utf8FirstByteOnes : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                char u[ 6 ];
                u[ 0 ] = 0xdc;
                u[ 1 ] = 0x80;
                
                u[ 2 ] = 0xef;
                u[ 3 ] = 0xbc;
                u[ 4 ] = 0x80;
                
                u[ 5 ] = 0;
                
                b.append( "a", u );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : \"\\u0700\\uff00\" }";
            }
        };
        
        class DBRef : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                OID o;
                memset( &o, 0, 12 );
                b.appendDBRef( "a", "foo", o );
                return b.doneAndDecouple();
            }
            // NOTE Testing other formats handled by by Base class.
            virtual string json() const {
                return "{ \"a\" : { \"$ns\" : \"foo\", \"$id\" : \"000000000000000000000000\" } }";
            }
        };
        
        class Oid : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendOID( "_id" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"_id\" : \"000000000000000000000000\" }";
            }
        };
        
        class Oid2 : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                OID o;
                memset( &o, 0x0f, 12 );
                b.appendOID( "_id", &o );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"_id\" : \"0f0f0f0f0f0f0f0f0f0f0f0f\" }";
            }
        };

        class BinData : public Base {
            virtual BSONObj bson() const {
                char z[ 3 ];
                z[ 0 ] = 'a';
                z[ 1 ] = 'b';
                z[ 2 ] = 'c';
                BSONObjBuilder b;
                b.appendBinData( "a", 3, ByteArray, z );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$binary\" : \"YWJj\", \"$type\" : \"02\" } }";
            }
        };
        
        class BinDataPaddedSingle : public Base {
            virtual BSONObj bson() const {
                char z[ 2 ];
                z[ 0 ] = 'a';
                z[ 1 ] = 'b';
                BSONObjBuilder b;
                b.appendBinData( "a", 2, ByteArray, z );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$binary\" : \"YWI=\", \"$type\" : \"02\" } }";
            }
        };
        
        class BinDataPaddedDouble : public Base {
            virtual BSONObj bson() const {
                char z[ 1 ];
                z[ 0 ] = 'a';
                BSONObjBuilder b;
                b.appendBinData( "a", 1, ByteArray, z );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$binary\" : \"YQ==\", \"$type\" : \"02\" } }";
            }
        };
        
        class BinDataAllChars : public Base {
            virtual BSONObj bson() const {
                char z[] = {
                    0x00, 0x10, 0x83, 0x10, 0x51, 0x87, 0x20, 0x92, 0x8B, 0x30,
                    0xD3, 0x8F, 0x41, 0x14, 0x93, 0x51, 0x55, 0x97, 0x61, 0x96,
                    0x9B, 0x71, 0xD7, 0x9F, 0x82, 0x18, 0xA3, 0x92, 0x59, 0xA7,
                    0xA2, 0x9A, 0xAB, 0xB2, 0xDB, 0xAF, 0xC3, 0x1C, 0xB3, 0xD3,
                    0x5D, 0xB7, 0xE3, 0x9E, 0xBB, 0xF3, 0xDF, 0xBF
                };
                BSONObjBuilder b;
                b.appendBinData( "a", 48, ByteArray, z );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$binary\" : \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\", \"$type\" : \"02\" } }";
            }
        };
        
        class Date : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendDate( "a", 0 );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$date\" : 0 } }";
            }
        };
        
        class DateNonzero : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendDate( "a", 100 );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$date\" : 100 } }";
            }
        };
        
        class DateTooLong : public Bad {
            virtual string json() const {
                stringstream ss;
                ss << "{ \"a\" : { \"$date\" : " << ~(0LL) << "0" << " } }";
                return ss.str();
            }
        };
        
        class Regex : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendRegex( "a", "b", "i" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$regex\" : \"b\", \"$options\" : \"i\" } }";
            }
        };
        
        class RegexEscape : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendRegex( "a", "\t", "i" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : { \"$regex\" : \"\\t\", \"$options\" : \"i\" } }";
            }
        };
        
        class RegexWithQuotes : public Base {
            virtual BSONObj bson() const {
                BSONObjBuilder b;
                b.appendRegex( "a", "\"", "" );
                return b.doneAndDecouple();
            }
            virtual string json() const {
                return "{ \"a\" : /\"/ }";
            }
        };
        
        class RegexInvalidOption : public Bad {
            virtual string json() const {
                return "{ \"a\" : { \"$regex\" : \"b\", \"$options\" : \"1\" } }";
            }
        };
        
        class RegexInvalidOption2 : public Bad {
            virtual string json() const {
                return "{ \"a\" : /b/c }";
            }
        };
        
        class Malformed : public Bad {
            string json() const {
                return "{";
            }
        };
        
    } // namespace FromJsonTests
    
    class All : public UnitTest::Suite {
    public:
        All() {
            add< JsonStringTests::Empty >();
            add< JsonStringTests::SingleStringMember >();
            add< JsonStringTests::EscapedCharacters >();
            add< JsonStringTests::AdditionalControlCharacters >();
            add< JsonStringTests::ExtendedAscii >();
            add< JsonStringTests::EscapeFieldName >();
            add< JsonStringTests::SingleIntMember >();
            add< JsonStringTests::SingleNumberMember >();
            add< JsonStringTests::InvalidNumbers >();
            add< JsonStringTests::NumberPrecision >();
            add< JsonStringTests::NegativeNumber >();
            add< JsonStringTests::SingleBoolMember >();
            add< JsonStringTests::SingleNullMember >();
            add< JsonStringTests::SingleObjectMember >();
            add< JsonStringTests::TwoMembers >();
            add< JsonStringTests::EmptyArray >();
            add< JsonStringTests::Array >();
            add< JsonStringTests::DBRef >();
            add< JsonStringTests::DBRefZero >();
            add< JsonStringTests::ObjectId >();
            add< JsonStringTests::BinData >();
            add< JsonStringTests::Symbol >();
            add< JsonStringTests::Date >();
            add< JsonStringTests::Regex >();
            add< JsonStringTests::RegexEscape >();
            add< JsonStringTests::RegexManyOptions >();
            add< FromJsonTests::Empty >();
            add< FromJsonTests::EmptyWithSpace >();
            add< FromJsonTests::SingleString >();
            add< FromJsonTests::EmptyStrings >();
            add< FromJsonTests::ReservedFieldName >();
            add< FromJsonTests::OkDollarFieldName >();
            add< FromJsonTests::SingleNumber >();
            add< FromJsonTests::FancyNumber >();
            add< FromJsonTests::TwoElements >();
            add< FromJsonTests::Subobject >();
            add< FromJsonTests::ArrayEmpty >();
            add< FromJsonTests::Array >();
            add< FromJsonTests::True >();
            add< FromJsonTests::False >();
            add< FromJsonTests::Null >();
            add< FromJsonTests::EscapedCharacters >();
            add< FromJsonTests::AllowedControlCharacter >();
            add< FromJsonTests::EscapeFieldName >();
            add< FromJsonTests::EscapedUnicodeToUtf8 >();
            add< FromJsonTests::Utf8AllOnes >();
            add< FromJsonTests::Utf8FirstByteOnes >();
            add< FromJsonTests::DBRef >();
            add< FromJsonTests::Oid >();
            add< FromJsonTests::Oid2 >();
            add< FromJsonTests::BinData >();
            add< FromJsonTests::BinDataPaddedSingle >();
            add< FromJsonTests::BinDataPaddedDouble >();
            add< FromJsonTests::BinDataAllChars >();
            add< FromJsonTests::Date >();
            add< FromJsonTests::DateNonzero >();
            add< FromJsonTests::DateTooLong >();
            add< FromJsonTests::Regex >();
            add< FromJsonTests::RegexEscape >();
            add< FromJsonTests::RegexWithQuotes >();
            add< FromJsonTests::RegexInvalidOption >();
            add< FromJsonTests::RegexInvalidOption2 >();
            add< FromJsonTests::Malformed >();            
        }
    };
    
} // namespace JsonTests

UnitTest::TestPtr jsonTests() {
    return UnitTest::createSuite< JsonTests::All >();
}