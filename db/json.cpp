// json.cpp

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

#include "stdafx.h"
#include "json.h"
#include "../util/builder.h"

using namespace boost::spirit;

namespace mongo {

    struct ObjectBuilder {
        BSONObjBuilder *back() {
            return builders.back().get();
        }
        // Storage for field names of elements within builders.back().
        const char *fieldName() {
            return fieldNames.back().c_str();
        }
        void push() {
            boost::shared_ptr< BSONObjBuilder > b( new BSONObjBuilder() );
            builders.push_back( b );
            fieldNames.push_back( "" );
            indexes.push_back( 0 );
        }
        BSONObj pop() {
            BSONObj ret = back()->obj();
            builders.pop_back();
            fieldNames.pop_back();
            indexes.pop_back();
            return ret;
        }
        void nameFromIndex() {
            fieldNames.back() = BSONObjBuilder::numStr( indexes.back() );
        }
        string popString() {
            string ret = ss.str();
            ss.str( "" );
            return ret;
        }
        // Cannot use auto_ptr because its copy constructor takes a non const reference.
        vector< boost::shared_ptr< BSONObjBuilder > > builders;
        vector< string > fieldNames;
        vector< int > indexes;
        stringstream ss;
        string ns;
        OID oid;
        string binData;
        BinDataType binDataType;
        string regex;
        string regexOptions;
        unsigned long long date;
    };

    struct objectStart {
        objectStart( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char &c ) const {
            b.push();
        }
        ObjectBuilder &b;
    };

    struct arrayStart {
        arrayStart( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char &c ) const {
            b.push();
            b.nameFromIndex();
        }
        ObjectBuilder &b;
    };

    struct arrayNext {
        arrayNext( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char &c ) const {
            ++b.indexes.back();
            b.nameFromIndex();
        }
        ObjectBuilder &b;
    };

    struct ch {
        ch( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char c ) const {
            b.ss << c;
        }
        ObjectBuilder &b;
    };

    struct chE {
        chE( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char c ) const {
            char o = '\0';
            switch ( c ) {
            case '\"':
                o = '\"';
                break;
            case '\'':
                o = '\'';
                break;
            case '\\':
                o = '\\';
                break;
            case '/':
                o = '/';
                break;
            case 'b':
                o = '\b';
                break;
            case 'f':
                o = '\f';
                break;
            case 'n':
                o = '\n';
                break;
            case 'r':
                o = '\r';
                break;
            case 't':
                o = '\t';
                break;
            default:
                assert( false );
            }
            b.ss << o;
        }
        ObjectBuilder &b;
    };

    namespace hex {
        int val( char c ) {
            if ( '0' <= c && c <= '9' )
                return c - '0';
            if ( 'a' <= c && c <= 'f' )
                return c - 'a' + 10;
            if ( 'A' <= c && c <= 'F' )
                return c - 'A' + 10;
            assert( false );
            return 0xff;
        }
        char val( const char *c ) {
            return ( val( c[ 0 ] ) << 4 ) | val( c[ 1 ] );
        }
    } // namespace hex

    struct chU {
        chU( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            unsigned char first = hex::val( start );
            unsigned char second = hex::val( start + 2 );
            if ( first == 0 && second < 0x80 )
                b.ss << second;
            else if ( first < 0x08 ) {
                b.ss << char( 0xc0 | ( ( first << 2 ) | ( second >> 6 ) ) );
                b.ss << char( 0x80 | ( ~0xc0 & second ) );
            } else {
                b.ss << char( 0xe0 | ( first >> 4 ) );
                b.ss << char( 0x80 | ( ~0xc0 & ( ( first << 2 ) | ( second >> 6 ) ) ) );
                b.ss << char( 0x80 | ( ~0xc0 & second ) );
            }
        }
        ObjectBuilder &b;
    };

    struct chClear {
        chClear( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char c ) const {
            b.popString();
        }
        ObjectBuilder &b;
    };

    struct fieldNameEnd {
        fieldNameEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            string name = b.popString();
            massert( "Invalid use of reserved field name",
                     name != "$ns" &&
                     name != "$id" &&
                     name != "$binary" &&
                     name != "$type" &&
                     name != "$date" &&
                     name != "$regex" &&
                     name != "$options" );
            b.fieldNames.back() = name;
        }
        ObjectBuilder &b;
    };

    struct unquotedFieldNameEnd {
        unquotedFieldNameEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            string name( start, end );
            b.fieldNames.back() = name;
        }
        ObjectBuilder &b;
    };

    struct stringEnd {
        stringEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->append( b.fieldName(), b.popString() );
        }
        ObjectBuilder &b;
    };

    struct numberValue {
        numberValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( double d ) const {
            b.back()->append( b.fieldName(), d );
        }
        ObjectBuilder &b;
    };

    struct subobjectEnd {
        subobjectEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            BSONObj o = b.pop();
            b.back()->append( b.fieldName(), o );
        }
        ObjectBuilder &b;
    };

    struct arrayEnd {
        arrayEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            BSONObj o = b.pop();
            b.back()->appendArray( b.fieldName(), o );
        }
        ObjectBuilder &b;
    };

    struct trueValue {
        trueValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendBool( b.fieldName(), true );
        }
        ObjectBuilder &b;
    };

    struct falseValue {
        falseValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendBool( b.fieldName(), false );
        }
        ObjectBuilder &b;
    };

    struct nullValue {
        nullValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendNull( b.fieldName() );
        }
        ObjectBuilder &b;
    };

    struct dbrefNS {
        dbrefNS( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.ns = b.popString();
        }
        ObjectBuilder &b;
    };

// NOTE s must be 24 characters.
    OID stringToOid( const char *s ) {
        OID oid;
        char *oidP = (char *)( &oid );
        for ( int i = 0; i < 12; ++i )
            oidP[ i ] = hex::val( s + ( i * 2 ) );
        return oid;
    }

    struct oidValue {
        oidValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.oid = stringToOid( start );
        }
        ObjectBuilder &b;
    };

    struct dbrefEnd {
        dbrefEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendDBRef( b.fieldName(), b.ns.c_str(), b.oid );
        }
        ObjectBuilder &b;
    };

    struct oidEnd {
        oidEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendOID( "_id", &b.oid );
        }
        ObjectBuilder &b;
    };

// NOTE The boost base64 library code was originally written for use only by the
// boost::archive package, however a google search reveals that these base64
// routines are used in a lot of non-boost code as well.  The library can't
// handle '=' padding bytes, so here I replace them with 'A' (the value for 0
// in base64's 6bit encoding) and then drop the garbage zeroes produced by
// boost's conversion.
    struct binDataBinary {
        typedef
        boost::archive::iterators::transform_width
        < boost::archive::iterators::binary_from_base64
        < string::const_iterator >, 8, 6
        > binary_t;
        binDataBinary( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            massert( "Badly formatted bindata", ( end - start ) % 4 == 0 );
            string base64( start, end );
            int len = base64.length();
            int pad = 0;
            for (; len - pad > 0 && base64[ len - 1 - pad ] == '='; ++pad )
                base64[ len - 1 - pad ] = 'A';
            massert( "Badly formatted bindata", pad < 3 );
            b.binData = string( binary_t( base64.begin() ), binary_t( base64.end() ) );
            b.binData.resize( b.binData.length() - pad );
        }
        ObjectBuilder &b;
    };

    struct binDataType {
        binDataType( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.binDataType = BinDataType( hex::val( start ) );
        }
        ObjectBuilder &b;
    };

    struct binDataEnd {
        binDataEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendBinData( b.fieldName(), b.binData.length(),
                                     b.binDataType, b.binData.data() );
        }
        ObjectBuilder &b;
    };

    struct dateValue {
        dateValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( unsigned long long v ) const {
            b.date = v;
        }
        ObjectBuilder &b;
    };

    struct dateEnd {
        dateEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendDate( b.fieldName(), b.date );
        }
        ObjectBuilder &b;
    };

    struct regexValue {
        regexValue( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.regex = b.popString();
        }
        ObjectBuilder &b;
    };

    struct regexOptions {
        regexOptions( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.regexOptions = string( start, end );
        }
        ObjectBuilder &b;
    };

    struct regexEnd {
        regexEnd( ObjectBuilder &_b ) : b( _b ) {}
        void operator() ( const char *start, const char *end ) const {
            b.back()->appendRegex( b.fieldName(), b.regex.c_str(),
                                   b.regexOptions.c_str() );
        }
        ObjectBuilder &b;
    };

// One gotcha with this parsing library is probably best ilustrated with an
// example.  Say we have a production like this:
// z = ( ch_p( 'a' )[ foo ] >> ch_p( 'b' ) ) | ( ch_p( 'a' )[ foo ] >> ch_p( 'c' ) );
// On input "ac", action foo() will be called twice -- once as the parser tries
// to match "ab", again as the parser successfully matches "ac".  Sometimes
// the grammar can be modified to eliminate these situations.  Here, for example:
// z = ch_p( 'a' )[ foo ] >> ( ch_p( 'b' ) | ch_p( 'c' ) );
// However, this is not always possible.  In my implementation I've tried to
// stick to the following pattern: store fields fed to action callbacks
// temporarily as ObjectBuilder members, then append to a BSONObjBuilder once
// the parser has completely matched a nonterminal and won't backtrack.  It's
// worth noting here that this parser follows a short-circuit convention.  So,
// in the original z example on line 3, if the input was "ab", foo() would only
// be called once.
    struct JsonGrammar : public grammar< JsonGrammar > {
public:
        JsonGrammar( ObjectBuilder &_b ) : b( _b ) {}

        template < typename ScannerT >
        struct definition {
            definition( JsonGrammar const &self ) {
                object = ch_p( '{' )[ objectStart( self.b ) ] >> !members >> '}';
                members = pair >> !( ',' >> members );
                pair =
                    oid[ oidEnd( self.b ) ] |
                    fieldName >> ':' >> value;
                fieldName =
                    str[ fieldNameEnd( self.b ) ] |
                    singleQuoteStr[ fieldNameEnd( self.b ) ] |
                    unquotedFieldName[ unquotedFieldNameEnd( self.b ) ];
                array = ch_p( '[' )[ arrayStart( self.b ) ] >> !elements >> ']';
                elements = value >> !( ch_p( ',' )[ arrayNext( self.b ) ] >> elements );
                value =
                    dbref[ dbrefEnd( self.b ) ] |
                    bindata[ binDataEnd( self.b ) ] |
                    date[ dateEnd( self.b ) ] |
                    regex[ regexEnd( self.b ) ] |
                    str[ stringEnd( self.b ) ] |
                    singleQuoteStr[ stringEnd( self.b ) ] |
                    number |
                    object[ subobjectEnd( self.b ) ] |
                    array[ arrayEnd( self.b ) ] |
                    lexeme_d[ str_p( "true" ) ][ trueValue( self.b ) ] |
                    lexeme_d[ str_p( "false" ) ][ falseValue( self.b ) ] |
                    lexeme_d[ str_p( "null" ) ][ nullValue( self.b ) ];
                // NOTE lexeme_d and rules don't mix well, so we have this mess.
                // NOTE We use range_p rather than cntrl_p, because the latter is locale dependent.
                str = lexeme_d[ ch_p( '"' )[ chClear( self.b ) ] >>
                                *( ( ch_p( '\\' ) >>
                                     ( ch_p( '"' )[ chE( self.b ) ] |
                                       ch_p( '\\' )[ chE( self.b ) ] |
                                       ch_p( '/' )[ chE( self.b ) ] |
                                       ch_p( 'b' )[ chE( self.b ) ] |
                                       ch_p( 'f' )[ chE( self.b ) ] |
                                       ch_p( 'n' )[ chE( self.b ) ] |
                                       ch_p( 'r' )[ chE( self.b ) ] |
                                       ch_p( 't' )[ chE( self.b ) ] |
                                       ( ch_p( 'u' ) >> ( repeat_p( 4 )[ xdigit_p ][ chU( self.b ) ] ) ) ) ) |
                                   ( ~range_p( 0x00, 0x1f ) & ~ch_p( '"' ) & ( ~ch_p( '\\' ) )[ ch( self.b ) ] ) ) >> '"' ];

                singleQuoteStr = lexeme_d[ ch_p( '\'' )[ chClear( self.b ) ] >>
                                *( ( ch_p( '\\' ) >>
                                     ( ch_p( '\'' )[ chE( self.b ) ] |
                                       ch_p( '\\' )[ chE( self.b ) ] |
                                       ch_p( '/' )[ chE( self.b ) ] |
                                       ch_p( 'b' )[ chE( self.b ) ] |
                                       ch_p( 'f' )[ chE( self.b ) ] |
                                       ch_p( 'n' )[ chE( self.b ) ] |
                                       ch_p( 'r' )[ chE( self.b ) ] |
                                       ch_p( 't' )[ chE( self.b ) ] |
                                       ( ch_p( 'u' ) >> ( repeat_p( 4 )[ xdigit_p ][ chU( self.b ) ] ) ) ) ) |
                                   ( ~range_p( 0x00, 0x1f ) & ~ch_p( '\'' ) & ( ~ch_p( '\\' ) )[ ch( self.b ) ] ) ) >> '\'' ];

                // real_p accepts numbers with nonsignificant zero prefixes, which
                // aren't allowed in JSON.  Oh well.
                number = real_p[ numberValue( self.b ) ];
                
                // We allow a subset of valid js identifier names here.
                unquotedFieldName = lexeme_d[ ( alpha_p | ch_p( '$' ) ) >> *( ( alnum_p | ch_p( '$' ) | ch_p( '_' ) ) ) ];

                dbref = dbrefS | dbrefT;
                dbrefS = ch_p( '{' ) >> "\"$ns\"" >> ':' >>
                         str[ dbrefNS( self.b ) ] >> ',' >> "\"$id\"" >> ':' >> quotedOid >> '}';
                dbrefT = str_p( "Dbref" ) >> '(' >> str[ dbrefNS( self.b ) ] >> ',' >>
                         quotedOid >> ')';

                // FIXME Only object id if top level field?
                oid = oidS | oidT;
                oidS = str_p( "\"_id\"" ) >> ':' >> quotedOid;
                oidT = str_p( "\"_id\"" ) >> ':' >> "ObjectId" >> '(' >> quotedOid >> ')';

                quotedOid = lexeme_d[ '"' >> ( repeat_p( 24 )[ xdigit_p ] )[ oidValue( self.b ) ] >> '"' ];

                bindata = ch_p( '{' ) >> "\"$binary\"" >> ':' >>
                          lexeme_d[ '"' >> ( *( range_p( 'A', 'Z' ) | range_p( 'a', 'z' ) | range_p( '0', '9' ) | ch_p( '+' ) | ch_p( '/' ) ) >> *ch_p( '=' ) )[ binDataBinary( self.b ) ] >> '"' ] >> ',' >> "\"$type\"" >> ':' >>
                          lexeme_d[ '"' >> ( repeat_p( 2 )[ xdigit_p ] )[ binDataType( self.b ) ] >> '"' ] >> '}';

                date = dateS | dateT;
                dateS = ch_p( '{' ) >> "\"$date\"" >> ':' >> uint_parser< unsigned long long >()[ dateValue( self.b ) ] >> '}';
                dateT = str_p( "Date" ) >> '(' >> uint_parser< unsigned long long >()[ dateValue( self.b ) ] >> ')';

                regex = regexS | regexT;
                regexS = ch_p( '{' ) >> "\"$regex\"" >> ':' >> str[ regexValue( self.b ) ] >> ',' >> "\"$options\"" >> ':' >> lexeme_d[ '"' >> ( *( alpha_p ) )[ regexOptions( self.b ) ] >> '"' ] >> '}';
                // FIXME Obviously it would be nice to unify this with str.
                regexT = lexeme_d[ ch_p( '/' )[ chClear( self.b ) ] >>
                                   *( ( ch_p( '\\' ) >>
                                        ( ch_p( '"' )[ chE( self.b ) ] |
                                          ch_p( '\\' )[ chE( self.b ) ] |
                                          ch_p( '/' )[ chE( self.b ) ] |
                                          ch_p( 'b' )[ chE( self.b ) ] |
                                          ch_p( 'f' )[ chE( self.b ) ] |
                                          ch_p( 'n' )[ chE( self.b ) ] |
                                          ch_p( 'r' )[ chE( self.b ) ] |
                                          ch_p( 't' )[ chE( self.b ) ] |
                                          ( ch_p( 'u' ) >> ( repeat_p( 4 )[ xdigit_p ][ chU( self.b ) ] ) ) ) ) |
                                      ( ~range_p( 0x00, 0x1f ) & ~ch_p( '/' ) & ( ~ch_p( '\\' ) )[ ch( self.b ) ] ) ) >> str_p( "/" )[ regexValue( self.b ) ]
                                   >> ( *( ch_p( 'i' ) | ch_p( 'g' ) | ch_p( 'm' ) ) )[ regexOptions( self.b ) ] ];
            }
            rule< ScannerT > object, members, pair, array, elements, value, str, number,
            dbref, dbrefS, dbrefT, oid, oidS, oidT, bindata, date, dateS, dateT,
            regex, regexS, regexT, quotedOid, fieldName, unquotedFieldName, singleQuoteStr;
            const rule< ScannerT > &start() const {
                return object;
            }
        };
        ObjectBuilder &b;
    };

    BSONObj fromjson( const char *str ) {
        if ( ! strlen(str) )
            return BSONObj();
        ObjectBuilder b;
        JsonGrammar parser( b );
        parse_info<> result = parse( str, parser, space_p );
        if ( !result.full ) {
            int len = strlen( result.stop );
            if ( len > 10 )
                len = 10;
            stringstream ss;
            ss << "Failure parsing JSON string near: " << string( result.stop, len );
            massert( ss.str(), false );
        }
        return b.pop();
    }

    BSONObj fromjson( const string &str ) {
        return fromjson( str.c_str() );
    }

} // namespace mongo
