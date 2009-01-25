// matchertests.cpp : matcher unit tests
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

#include "../db/matcher.h"

#include "../db/json.h"

#include "dbtests.h"

namespace MatcherTests {

    class Basic {
    public:
        void run() {
            BSONObj query = fromjson( "{\"a\":\"b\"}" );
            JSMatcher m( query, fromjson( "{\"a\":1}" ) );
            ASSERT( m.matches( fromjson( "{\"a\":\"b\"}" ) ) );
        }
    };
    
    class DoubleEqual {
    public:
        void run() {
            BSONObj query = fromjson( "{\"a\":5}" );
            JSMatcher m( query, fromjson( "{\"a\":1}" ) );
            ASSERT( m.matches( fromjson( "{\"a\":5}" ) ) );            
        }
    };
    
    class MixedNumericEqual {
    public:
        void run() {
            BSONObjBuilder query;
            query.appendInt( "a", 5 );
            JSMatcher m( query.done(), fromjson( "{\"a\":1}" ) );
            ASSERT( m.matches( fromjson( "{\"a\":5}" ) ) );            
        }        
    };
    
    class MixedNumericGt {
    public:
        void run() {
            BSONObj query = fromjson( "{\"a\":{\"$gt\":4}}" );
            JSMatcher m( query, fromjson( "{\"a\":1}" ) );
            BSONObjBuilder b;
            b.appendInt( "a", 5 );
            ASSERT( m.matches( b.done() ) );
        }        
    };

    class All : public UnitTest::Suite {
    public:
        All() {
            add< Basic >();
            add< DoubleEqual >();
            add< MixedNumericEqual >();
            add< MixedNumericGt >();
        }
    };
    
} // namespace MatcherTests

UnitTest::TestPtr matcherTests() {
    return UnitTest::createSuite< MatcherTests::All >();
}