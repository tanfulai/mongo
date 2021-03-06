// javajstests.cpp : Btree unit tests
//

/**
 *    Copyright (C) 2009 10gen Inc.
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

#include "../db/javajs.h"

#include "dbtests.h"

namespace JavaJSTests {

    class Fundamental {
    public:
        void run() {
            // By calling JavaJSImpl() inside run(), we ensure the unit test framework's
            // signal handlers are pre-installed from JNI's perspective.  This allows
            // JNI to catch signals generated within the JVM and forward other signals
            // as appropriate.
            JavaJS = new JavaJSImpl();
            javajstest();
        }
    };
    
    class All : public UnitTest::Suite {
    public:
        All() {
            add< Fundamental >();
        }
    };
    
} // namespace JavaJSTests

UnitTest::TestPtr javajsTests() {
    return UnitTest::createSuite< JavaJSTests::All >();
}
